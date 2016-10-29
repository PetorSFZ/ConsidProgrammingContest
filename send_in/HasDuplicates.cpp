// Copyright(c) Peter Hillerström (peter.hillerstrom93@gmail.com, skipifzero.com)

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <malloc.h>

using namespace std;

// Constants
// ------------------------------------------------------------------------------------------------

static const uint64_t MAX_NUMBER_CODES = 17576000;
static const uint64_t BYTES_PER_CODE = 8; // Assumes Windows file endings (2 bytes per newline)
static const uint64_t NUM_BITSET_BYTES = (MAX_NUMBER_CODES / 8);

static const uint64_t NUM_THREADS = 3; // NOTE: This can not be changed, it is very hardcoded
static const uint64_t CODE_ALLOCATION_BATCH_SIZE = 4096;
static const uint64_t NUM_CODES_MULTI_THREADED_THRESHOLD = 600000;

// Single threaded variant
// ------------------------------------------------------------------------------------------------

static bool singleThreadedSearch(const uint8_t* __restrict fileView, uint64_t fileSize) noexcept
{
	// Allocate bitset buffer for whether a number is found or not, and clear it
	uint64_t* isFoundBitset = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	for (size_t i = 0; i < fileSize; i += 8) {
		char let3 = fileView[i];
		char let2 = fileView[i + 1];
		char let1 = fileView[i + 2];
		char no3 = fileView[i + 3];
		char no2 = fileView[i + 4];
		char no1 = fileView[i + 5];

		// Calculate corresponding number for code
		uint32_t number = uint32_t(let3 - 'A') * 676000u +
		                  uint32_t(let2 - 'A') * 26000u +
		                  uint32_t(let1 - 'A') * 1000u +
		                  uint32_t(no3 - '0') * 100u +
		                  uint32_t(no2 - '0') * 10u +
		                  uint32_t(no1 - '0');

		uint32_t bitsetChunkIndex = number >> 6u; // number / 64;
		uint32_t bitIndex = number & 0x0000003Fu; // number % 64;

		uint64_t chunk = isFoundBitset[bitsetChunkIndex];
		uint64_t bitMask = uint64_t(1) << bitIndex;
		
		if ((bitMask & chunk) != uint64_t(0)) {
			foundCopy = true;
			break;
		}

		chunk = bitMask | chunk;
		isFoundBitset[bitsetChunkIndex] = chunk;
	}

	// Free allocated bitset memory
	_aligned_free(isFoundBitset);

	// Return result
	return foundCopy;
}

// Multi-threaded variant
// ------------------------------------------------------------------------------------------------

static void workerFunction(const uint8_t* __restrict fileView,
                           uint64_t* __restrict isFoundBitset,
                           bool* foundCopy,
                           size_t numCodes,
                           atomic_size_t* nextFreeCodeIndex) noexcept
{
	// Clear bitset
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	while (true) {

		// Allocate codes from shared array
		size_t codeIndex = nextFreeCodeIndex->fetch_add(CODE_ALLOCATION_BATCH_SIZE);
		if (codeIndex >= numCodes) return;
		size_t codesToCheck = min(size_t(CODE_ALLOCATION_BATCH_SIZE), numCodes - codeIndex);
		
		// Loop over all allocated codes
		size_t start = codeIndex * BYTES_PER_CODE;
		size_t end = (codeIndex + codesToCheck) * BYTES_PER_CODE;
		for (size_t i = start; i < end; i += BYTES_PER_CODE) {
			
			uint8_t let3 = fileView[i];
			uint8_t let2 = fileView[i + 1];
			uint8_t let1 = fileView[i + 2];
			uint8_t num3 = fileView[i + 3];
			uint8_t num2 = fileView[i + 4];
			uint8_t num1 = fileView[i + 5];

			// Calculate corresponding number for code
			uint32_t number = uint32_t(let3 - 'A') * 676000u +
			                  uint32_t(let2 - 'A') * 26000u +
			                  uint32_t(let1 - 'A') * 1000u +
			                  uint32_t(num3 - '0') * 100u +
			                  uint32_t(num2 - '0') * 10u +
			                  uint32_t(num1 - '0');

			uint32_t bitsetChunkIndex = number >> 6u; // number / 64;
			uint32_t bitIndex = number & 0x0000003Fu; // number % 64;

			uint64_t chunk = isFoundBitset[bitsetChunkIndex];
			uint64_t bitMask = uint64_t(1) << bitIndex;
		
			if ((bitMask & chunk) != uint64_t(0)) {

				// Allocate rest of rays so the other threads can stop
				nextFreeCodeIndex->fetch_add(numCodes);

				// Signal that the copy is found and exit thread
				*foundCopy = true;
				return;
			}

			chunk = bitMask | chunk;
			isFoundBitset[bitsetChunkIndex] = chunk;
		}
	}
}

static bool multiThreadedSearch(const uint8_t* __restrict fileView, uint64_t numCodes) noexcept
{
	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	// Counter used for allocating codes
	atomic_size_t nextFreeCodeIndex = 0;

	// Start threads
	uint64_t* bitsets[NUM_THREADS];
	thread threads[NUM_THREADS];
	for (size_t i = 0; i < NUM_THREADS; i++) {

		// Allocate memory for bitset, cleared in worker function
		bitsets[i] = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));

		// Start worker thread
		threads[i] = thread(workerFunction, fileView, bitsets[i], &foundCopy, numCodes, &nextFreeCodeIndex);
	}

	// Wait for threads to finish working
	for (thread& t : threads) {
		t.join();
	}

	// Compare all thread tables
	if (!foundCopy) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			uint64_t b3 = bitsets[2][i];
			bool found = ((b1 & b2) | (b1 & b3) | (b2 & b3)) != uint64_t(0);
			if (found) {
				foundCopy = true;
				break;
			}
		}
	}

	// Free memory
	for (uint64_t* bitset : bitsets) {
		_aligned_free(bitset);
	}

	// Return result
	return foundCopy;
}

// hasDuplicates() entry functions
// ------------------------------------------------------------------------------------------------

static bool hasDuplicates(const char* filePath) noexcept
{
	// Open file
	HANDLE file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
	                         FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (!file) {
		printf("CreateFile() failed\n");
		return false;
	}

	// Get size of file
	uint64_t fileSize = static_cast<uint64_t>(GetFileSize(file, NULL));

	// Large file fast path
	// There are a total of 17 576 000 different codes. If the file contains more codes than that
	// it must also by definition contain a copy.
	if (fileSize >= ((MAX_NUMBER_CODES + 1) * BYTES_PER_CODE)) {
		CloseHandle(file);
		return true;
	}

	// Create mapped file
	HANDLE mappedFile = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!mappedFile) {
		printf("CreateFileMapping() failed\n");
		CloseHandle(file);
		return false;
	}

	// Create mapped file view
	uint64_t offset = 0;
	uint32_t offsetLow  = uint32_t(offset & uint64_t(0xFFFFFFFF));
	uint32_t offsetHigh = uint32_t(offset >> uint64_t(32));
	uint64_t numBytesToMap = fileSize;
	void* fileView = MapViewOfFile(mappedFile, FILE_MAP_READ, offsetLow, offsetHigh, numBytesToMap);
	if (!fileView) {
		printf("MapViewOfFile() failed\n");
		CloseHandle(mappedFile);
		CloseHandle(file);
		return false;
	}

	// Result variable
	bool foundCopy = false;

	// Single threaded path
	uint64_t numCodes = fileSize / BYTES_PER_CODE;
	if (numCodes <= NUM_CODES_MULTI_THREADED_THRESHOLD) {
		foundCopy = singleThreadedSearch(static_cast<const uint8_t*>(fileView), fileSize);
	}
	
	// Multi-threaded path
	else {
		foundCopy = multiThreadedSearch(static_cast<const uint8_t*>(fileView), numCodes);
	}

	// Unmap mapped file view
	if (!UnmapViewOfFile(fileView)) {
		printf("UnmapViewOfFile() failed\n");
		CloseHandle(mappedFile);
		CloseHandle(file);
		return false;
	}

	// Close mapped file
	if (!CloseHandle(mappedFile)) {
		printf("CloseHandle() failed for mappedFile\n");
		return false;
	}

	// Close file
	if (!CloseHandle(file)) {
		printf("CloseHandle() failed for file\n");
		return false;
	}

	// Return result
	return foundCopy;
}

// Main
// ------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	// Retrieve file path from input parameters
	if (argc != 2) {
		printf("Invalid arguments, proper usage: \"FindDuplicates <filename>\"\n");
		return 1;
	}
	const char* path = argv[1];

	// Check file for duplicates
	bool result = hasDuplicates(path);
	if (result) {
		printf("Dubbletter\n");
	} else {
		printf("Ej dubblett\n");
	}

	// Flush output and exit program
	fflush(stdout);
	return 0;
}
