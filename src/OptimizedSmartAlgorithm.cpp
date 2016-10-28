// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "NaiveSmartAlgorithm.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <malloc.h>

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

static __declspec(noalias) size_t readFile(const char* __restrict path, char* __restrict outBuffer) noexcept
{
	// Open file
	if (path == nullptr) return 0;
	FILE* file = fopen(path, "r");
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

/*static vector<uint32_t> parseFile(const vector<char>& contents) noexcept
{
	vector<uint32_t> numbers;
	numbers.reserve((contents.size() + 7) / 7);
	for (size_t i = 0; i < contents.size(); i += 7) {
		
		if ((i + 6) >= contents.size()) {
			break;
		}

		uint32_t reg[6];
		reg[0] = contents[i + 5] - '0';
		reg[1] = contents[i + 4] - '0';
		reg[2] = contents[i + 3] - '0';
		reg[3] = contents[i + 2] - 'A';
		reg[4] = contents[i + 1] - 'A';
		reg[5] = contents[i + 0] - 'A';
		
		uint32_t number = reg[5] * 676000u +
		                  reg[4] * 26000u +
		                  reg[3] * 1000u +
		                  reg[2] * 100u +
		                  reg[1] * 10u +
		                  reg[0];
		
		numbers.push_back(number);
	}
	return std::move(numbers);
}*/

// Exposed function
// ------------------------------------------------------------------------------------------------

bool optimizedSmartAlgorithm(const char* filePath) noexcept
{
	// Read file
	size_t tmpSize = sizeOfFile(filePath) + 16; // +16 so we guarantee SIMD writable
	char* fileContent = static_cast<char*>(_aligned_malloc(tmpSize, 32));
	size_t fileContentSize = readFile(filePath, fileContent);

	// Allocate bitset buffer for whether a number is found or not, and clear it
	const size_t NUM_BITSET_BYTES = 2197000;
	uint64_t* isFoundBitset = static_cast<uint64_t*>(_aligned_malloc(NUM_BITSET_BYTES, 32));
	memset(isFoundBitset, 0, NUM_BITSET_BYTES);

	// Variable containing whether a copy was found or not
	bool foundCopy = false;

	// Go through file
	for (size_t i = 5; i < fileContentSize; i += 7) {
		char no1 = fileContent[i];
		char no2 = fileContent[i-1];
		char no3 = fileContent[i-2];
		char let1 = fileContent[i-3];
		char let2 = fileContent[i-4];
		char let3 = fileContent[i-5];

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

	/*printf("\n\n");
	for (size_t i = 0; i < 9; i++) {
		printf("%c", fileContent[i]);
	}
	printf("\n\n");
	fflush(stdout);*/

	// Free allocated memory
	_aligned_free(isFoundBitset);
	_aligned_free(fileContent);

	return foundCopy;



	/*vector<char> contents = readFile(filePath);
	vector<uint32_t> numbers = parseFile(contents);

	// Allocate buffer and clear it
	const size_t NUM_BITSET_LONGS = 274625;
	uint64_t* buffer = new uint64_t[NUM_BITSET_LONGS];
	std::memset(buffer, 0, NUM_BITSET_LONGS * sizeof(uint64_t));

	bool foundCopy = false;
	for (uint32_t number : numbers) {

		uint32_t bitsetChunkIndex = number / 64;
		uint64_t bitIndex = number % 64;

		uint64_t chunk = buffer[bitsetChunkIndex];
		uint64_t bitMask = uint64_t(1) << bitIndex;
		
		bool exists = (bitMask & chunk) != 0;

		if (exists) {
			foundCopy = true;
			break;
		}

		chunk = bitMask | chunk;
		buffer[bitsetChunkIndex] = chunk;
	}

	delete[] buffer;
	return foundCopy;*/
}
