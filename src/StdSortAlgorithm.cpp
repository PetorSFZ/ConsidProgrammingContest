// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "StdSortAlgorithm.hpp"

#include <algorithm>
#include <vector>

using namespace std;

// Statics
// ------------------------------------------------------------------------------------------------

static vector<char> readFile(const char* path) noexcept
{
	// Open file
	if (path == nullptr) return vector<char>();
	std::FILE* file = std::fopen(path, "r");
	if (file == NULL) return vector<char>();

	// Get size of file
	std::fseek(file, 0, SEEK_END);
	int64_t size = std::ftell(file);
	std::rewind(file); // Rewind position to beginning of file
	if (size < 0) {
		std::fclose(file);
		return vector<char>();
	}

	// Create array with enough capacity to fit file
	vector<char> temp;
	temp.reserve(size + 1);

	// Read the file into the array
	char buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = std::fread(buffer, 1, BUFSIZ, file)) > 0) {
		for (size_t i = 0; i < readSize; i++) {
			temp.push_back(buffer[i]);
		}
	}

	std::fclose(file);
	return std::move(temp);
}

static vector<uint32_t> parseFile(const vector<char>& contents) noexcept
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
}

// Exposed function
// ------------------------------------------------------------------------------------------------

bool stdSortAlgorithm(const char* filePath) noexcept
{
	vector<char> contents = readFile(filePath);
	vector<uint32_t> numbers = parseFile(contents);

	// Sort numbers
	std::sort(numbers.begin(), numbers.end());

	uint32_t lastNumber = ~0u; // Start of with impossible start number
	for (uint32_t number : numbers) {
		if (lastNumber == number) {
			return true;
		}
		lastNumber = number;
	}
	return false;
}
