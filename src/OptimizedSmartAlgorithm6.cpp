// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "OptimizedSmartAlgorithm6.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <malloc.h>

#include <nmmintrin.h>

using namespace std;

// Constants
// ------------------------------------------------------------------------------------------------

static const uint64_t MAX_NUMBER_CODES = 17576000;
static const uint64_t BYTES_PER_CODE = 8; // Assumes Windows file endings (2 bytes per newline)
static const uint64_t NUM_BITSET_BYTES = MAX_NUMBER_CODES / 8;

static const uint64_t NUM_THREADS = 3;
//static const uint64_t CODE_ALLOCATION_BATCH_SIZE = 6000000 / (NUM_THREADS);
static const uint64_t CODE_ALLOCATION_BATCH_SIZE = 4096;
static const uint64_t NUM_CODES_MULTI_THREADED_THRESHOLD = 600000;
//static const uint64_t NUM_CODES_MULTI_THREADED_THRESHOLD = 8192;

// Single threaded variant
// ------------------------------------------------------------------------------------------------

static bool singleThreadedSearch(const uint8_t* __restrict fileView, uint64_t fileSize) noexcept
{
	// Allocate bitset buffer for whether a number is found or not, and clear it
	uint64_t* isFoundBitset = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	//alignas(16) uint8_t buffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//__m128i a = _mm_set_epi8(;

	for (size_t i = 0; i < fileSize; i += 8) {
		/*char let3 = fileView[i];
		char let2 = fileView[i + 1];
		char let1 = fileView[i + 2];
		char no3 = fileView[i + 3];
		char no2 = fileView[i + 4];
		char no1 = fileView[i + 5];*/

		// high [0, 0, 0, 0, 'Z', 'Z', '9', '9'] low, u16
		const __m128i raw = _mm_setr_epi16(fileView[i + 4], fileView[i + 3], fileView[i + 2], fileView[i + 1],
		                                   0, 0, 0, 0);
		
		// high [0, 0, 0, 0, 25, 25, 9, 9] low, u16
		const __m128i SUBTRACT_CHARS = _mm_setr_epi16('0', '0', 'A', 'A', 0, 0, 0, 0);
		const __m128i tmp1 = _mm_sub_epi16(raw, SUBTRACT_CHARS);

		// high [0, 0, Z*26000 + Z*1000, 9*100 + 9*10] low, u32
		const __m128i MULT_FACTORS = _mm_setr_epi16(10, 100, 1000, 26000, 0, 0, 0, 0);
		const __m128i tmp2 = _mm_madd_epi16(tmp1, MULT_FACTORS);

		// high [0, 0, 0, Z*26000 + Z*1000, 9*100 + 9*10] low, u32
		const __m128i tmp3 = _mm_add_epi32(tmp2, _mm_srli_si128(tmp2, 4));
		uint32_t tmp3Val = _mm_cvtsi128_si32(tmp3);
		
		// Calculate final number
		uint32_t number = uint32_t(fileView[i] - 'A') * 676000u + tmp3Val + uint32_t(fileView[i + 5] - '0');


		/*// [0, 0, ... 0, 'Z', 'Z', 'Z', '9', '9', '9'] (u8)
		const __m128i rawCode = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   fileView[i], fileView[i+1], fileView[i+2], fileView[i+3], fileView[i+4], fileView[i+5]);

		// [0, 0, ... , 0, 25, 25, 25, 9, 9, 9] (u8)
		const __m128i SUBTRACT_CHARS = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'A', 'A', 'A', '0', '0', '0');
		const __m128i partNumbers = _mm_sub_epi8(rawCode, SUBTRACT_CHARS);

		// [0, 9, 9, 9] (u32), extends "number" part of code to u32
		const __m128i LOW_PART_SHUFFLE_MASK = _mm_setr_epi8(0xFF, 0xFF, 0xFF, 0xFF,
		                                                    0xFF, 0xFF, 0xFF, 0x02,
		                                                    0xFF, 0xFF, 0xFF, 0x01,
		                                                    0xFF, 0xFF, 0xFF, 0x00);
		const __m128i lowPartNumbers = _mm_shuffle_epi8(partNumbers, LOW_PART_SHUFFLE_MASK);

		const __m128i LOW_PART_MULT_FACTORS = _mm_setr_epi32(0, 100, 10, 1);
		const __m128i lowPartMultplied = _mm_mul_epu32(lowPartNumbers, LOW_PART_MULT_FACTORS);*/


		// Calculate corresponding number for code
	/*	uint32_t number = uint32_t(let3 - 'A') * 676000u +
		                  uint32_t(let2 - 'A') * 26000u +
		                  uint32_t(let1 - 'A') * 1000u +
		                  uint32_t(no3 - '0') * 100u +
		                  uint32_t(no2 - '0') * 10u +
		                  uint32_t(no1 - '0');
*/
		uint32_t bitsetChunkIndex = number / 64;
		uint64_t bitIndex = number % 64;

		uint64_t chunk = isFoundBitset[bitsetChunkIndex];
		uint64_t bitMask = uint64_t(1) << bitIndex;
		
		bool exists = (bitMask & chunk) != 0;

		if (exists) {
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

static bool mergeBitsets(uint64_t* bitsets[NUM_THREADS]) noexcept
{
	if (NUM_THREADS == 2) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			if ((b1 & b2) != uint64_t(0)) return true;
		}
	}

	else if (NUM_THREADS == 3) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			uint64_t b3 = bitsets[2][i];
			bool found = ((b1 & b2) | (b1 & b3) | (b2 & b3)) != uint64_t(0);
			if (found) return true;
		}
	}

	else if (NUM_THREADS == 4) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			uint64_t b3 = bitsets[2][i];
			uint64_t b4 = bitsets[3][i];
			
			bool found = ((b1 & b2) | (b1 & b3) | (b1 & b4) | (b2 & b3) | (b2 & b4) | (b3 & b4)) != uint64_t(0);
			if (found) {
				return true;
			}
		}
	}
		
	else if (NUM_THREADS == 8) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			uint64_t b3 = bitsets[2][i];
			uint64_t b4 = bitsets[3][i];
			uint64_t b5 = bitsets[4][i];
			uint64_t b6 = bitsets[5][i];
			uint64_t b7 = bitsets[6][i];
			uint64_t b8 = bitsets[7][i];

			uint64_t val =
			(b1 & b2) |
			(b1 & b3) |
			(b1 & b4) |
			(b1 & b5) |
			(b1 & b6) |
			(b1 & b7) |
			(b1 & b8) |

			(b2 & b3) |
			(b2 & b4) |
			(b2 & b5) |
			(b2 & b6) |
			(b2 & b7) |
			(b2 & b8) |
			
			(b3 & b4) |
			(b3 & b5) |
			(b3 & b6) |
			(b3 & b7) |
			(b3 & b8) |

			(b4 & b5) |
			(b4 & b6) |
			(b4 & b7) |
			(b4 & b8) |
			
			(b5 & b6) |
			(b5 & b7) |
			(b5 & b8) |

			(b6 & b7) |
			(b6 & b8) |

			(b7 & b8);

			if (val != uint64_t(0)) {
				return true;
			}
		}
	}

	else {
		printf("FATAL ERROR: NUM_THREADS may only be 2, 3, 4 or 8\n");
	}

	return false;
}

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
		size_t codeIndex = atomic_fetch_add(nextFreeCodeIndex, CODE_ALLOCATION_BATCH_SIZE);
		if (codeIndex >= numCodes) return;
		size_t codesToCheck = min(size_t(CODE_ALLOCATION_BATCH_SIZE), numCodes - codeIndex);
		
		// Loop over all allocated codes
		size_t start = codeIndex * BYTES_PER_CODE;
		size_t end = (codeIndex + codesToCheck) * BYTES_PER_CODE;
		for (size_t i = start; i < end; i += BYTES_PER_CODE) {
			
			uint8_t let3 = fileView[i];
			uint8_t let2 = fileView[i + 1];
			uint8_t let1 = fileView[i + 2];
			uint8_t no3 = fileView[i + 3];
			uint8_t no2 = fileView[i + 4];
			uint8_t no1 = fileView[i + 5];

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

				// Allocate rest of rays so the other threads can stop
				atomic_fetch_add(nextFreeCodeIndex, numCodes);

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

	// Compare all threads tables
	if (!foundCopy) {
		foundCopy = mergeBitsets(bitsets);
	}

	// Free memory
	for (uint64_t* bitset : bitsets) {
		_aligned_free(bitset);
	}

	// Return result
	return foundCopy;
}

// Exposed function
// ------------------------------------------------------------------------------------------------

bool optimizedSmartAlgorithm6(const char* filePath) noexcept
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
