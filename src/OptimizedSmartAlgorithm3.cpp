// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "OptimizedSmartAlgorithm3.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <thread>

using namespace std;

// Constants
// ------------------------------------------------------------------------------------------------

#define NUM_BYTES_PER_CODE 8
#define NUM_BITSET_BYTES 2197000

#define NUM_THREADS 8

//#define CODE_ALLOCATION_BATCH_SIZE 8192
#define CODE_ALLOCATION_BATCH_SIZE 32768

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
                                               bool* __restrict foundCopy,
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
		size_t start = codeIndex * NUM_BYTES_PER_CODE;
		size_t end = (codeIndex + codesToCheck) * NUM_BYTES_PER_CODE;
		for (size_t i = start; i < end; i += NUM_BYTES_PER_CODE) {
			
			char let3 = file[i];
			char let2 = file[i + 1];
			char let1 = file[i + 2];
			char no3 = file[i + 3];
			char no2 = file[i + 4];
			char no1 = file[i + 5];

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

/*	// Clear bitset
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
	}*/
}

// Exposed function
// ------------------------------------------------------------------------------------------------

bool optimizedSmartAlgorithm3(const char* filePath) noexcept
{
	// Read file
	size_t tmpSize = sizeOfFile(filePath);
	char* file = static_cast<char*>(_aligned_malloc(tmpSize, 32));
	size_t fileSize = readFile(filePath, file);
	size_t numCodes = fileSize / NUM_BYTES_PER_CODE;

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
		threads[i] = thread(workerFunction, file, bitsets[i], &foundCopy, numCodes, &nextFreeCodeIndex);
	}

	// Wait for threads to finish working
	for (thread& t : threads) {
		t.join();
	}

	// Compare all threads tables
	if (!foundCopy) {
		for (size_t i = 0; i < (NUM_BITSET_BYTES / sizeof(uint64_t)); i++) {
			
#if NUM_THREADS == 4
			uint64_t b1 = bitsets[0][i];
			uint64_t b2 = bitsets[1][i];
			uint64_t b3 = bitsets[2][i];
			uint64_t b4 = bitsets[3][i];
			
			bool found = ((b1 & b2) | (b1 & b3) | (b1 & b4) | (b2 & b3) | (b2 & b4) | (b3 & b4)) != uint64_t(0);
			if (found) {
				foundCopy = true;
				break;
			}
#elif NUM_THREADS == 8
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
				foundCopy = true;
				break;
			}
#else
#error Not implemented
#endif
		}
	}

	// Free memory
	for (uint64_t* bitset : bitsets) {
		_aligned_free(bitset);
	}

	// Return result
	return foundCopy;
}
