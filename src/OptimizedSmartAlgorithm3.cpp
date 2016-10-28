// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "OptimizedSmartAlgorithm3.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <thread>

using namespace std;

// Statics
// ------------------------------------------------------------------------------------------------

static __declspec(noalias) size_t sizeOfFile(const char* __restrict path) noexcept
{
	FILE* file = fopen(path, "rb");
	if (file == NULL) return 0;
	fseek(file, 0, SEEK_END);
	int64_t size = ftell(file);
	fclose(file);
	return size_t(size);
}

static __declspec(noalias)size_t readFile(const char* __restrict path, char* __restrict outBuffer) noexcept
{
	// Open file
	if (path == nullptr) return 0;
	FILE* file = fopen(path, "rb");
	if (file == NULL) return 0;

	// Get size of file
	fseek(file, 0, SEEK_END);
	int64_t size = ftell(file);
	rewind(file); // Rewind position to beginning of file
	if (size < 0) {
		fclose(file);
		return 0;
	}

	// Read the file into buffer
	char buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = fread(buffer, 1, BUFSIZ, file)) > 0) {
		memcpy(outBuffer + currOffs, buffer, readSize);
		currOffs += readSize;
	}

	// Close file and return number of bytes read
	fclose(file);
	return currOffs;
}

static __declspec(noalias) void workerFunction(const char* __restrict file,
                                               uint64_t* __restrict isFoundBitset,
                                               bool* __restrict foundCopy) noexcept
{
	// Clear bitset
	const size_t NUM_BITSET_BYTES = 2197000;
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	// Open file
	FILE* file = fopen(filePath, "rb");
	if (file == NULL) return;

	// Read through the file
	const size_t BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	size_t readSize;
	while ((readSize = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
		for (size_t i = 0; i < readSize; i += 8) {
			
			char let3 = buffer[i];
			char let2 = buffer[i + 1];
			char let1 = buffer[i + 2];
			char no3 = buffer[i + 3];
			char no2 = buffer[i + 4];
			char no1 = buffer[i + 5];

			// Calculate corresponding number for code
			uint32_t number = uint32_t(let3 - 'A') * 676000u +
			                  uint32_t(let2 - 'A') * 26000u +
			                  uint32_t(let1 - 'A') * 1000u +
			                  uint32_t(no3 - '0') * 100u +
			                  uint32_t(no2 - '0') * 10u +
			                  uint32_t(no1 - '0');

			uint32_t bitsetChunkIndex = number / 64;
			uint64_t bitIndex = number % 64;

			uint64_t chunk = isFoundBitset[bitsetChunkIndex];
			uint64_t bitMask = uint64_t(1) << bitIndex;
		
			bool exists = (bitMask & chunk) != 0;

			if (exists) {
				*foundCopy = true;
				break;
			}

			chunk = bitMask | chunk;
			isFoundBitset[bitsetChunkIndex] = chunk;
		}
	}
}

// Exposed function
// ------------------------------------------------------------------------------------------------

bool optimizedSmartAlgorithm3(const char* filePath) noexcept
{
	// Read file
	size_t tmpSize = sizeOfFile(filePath) + 16; // +16 so we guarantee SIMD readable
	char* file = static_cast<char*>(_aligned_malloc(tmpSize, 32));
	size_t fileSize = readFile(filePath, file);


	const size_t NUM_THREADS = 1;

	

	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	// Start threads
	uint64_t* bitsets[NUM_THREADS];
	thread threads[NUM_THREADS];
	for (size_t i = 0; i < NUM_THREADS; i++) {

		// Allocate memory for bitset, cleared in worker function
		const size_t NUM_BITSET_BYTES = 2197000;
		bitsets[i] = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));

		// Start worker thread
		threads[i] = thread(workerFunction, filePath, bitsets[i], &foundCopy);
	}

	// Wait for threads to finish working
	for (thread& t : threads) {
		t.join();
	}

	// Free memory
	for (uint64_t* bitset : bitsets) {
		_aligned_free(bitset);
	}

	// Return result
	return foundCopy;
}
