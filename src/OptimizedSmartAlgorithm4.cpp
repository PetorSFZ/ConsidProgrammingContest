// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "OptimizedSmartAlgorithm4.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <Windows.h>
#include <malloc.h>

using namespace std;

// Statics
// ------------------------------------------------------------------------------------------------


// Exposed function
// ------------------------------------------------------------------------------------------------

bool optimizedSmartAlgorithm4(const char* filePath) noexcept
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

	// Allocate bitset buffer for whether a number is found or not, and clear it
	const size_t NUM_BITSET_BYTES = 2197000;
	uint64_t* isFoundBitset = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	const char* fileViewChar = static_cast<const char*>(fileView);
	for (size_t i = 0; i < fileSize; i += 8) {
		char let3 = fileViewChar[i];
		char let2 = fileViewChar[i + 1];
		char let1 = fileViewChar[i + 2];
		char no3 = fileViewChar[i + 3];
		char no2 = fileViewChar[i + 4];
		char no1 = fileViewChar[i + 5];

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
			foundCopy = true;
			break;
		}

		chunk = bitMask | chunk;
		isFoundBitset[bitsetChunkIndex] = chunk;
	}

	// Free allocated bitset memory
	_aligned_free(isFoundBitset);

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
