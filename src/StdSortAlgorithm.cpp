// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include "StdSortAlgorithm.hpp"

// Statics
// ------------------------------------------------------------------------------------------------

/*template<typename T>
static DynArray<T> readFileInternal(const char* path, bool binaryMode) noexcept
{
	// Open file
	if (path == nullptr) return DynArray<T>();
	std::FILE* file = std::fopen(path, binaryMode ? "rb" : "r");
	if (file == NULL) return DynArray<T>();

	// Get size of file
	std::fseek(file, 0, SEEK_END);
	int64_t size = std::ftell(file);
	std::rewind(file); // Rewind position to beginning of file
	if (size < 0) {
		std::fclose(file);
		return DynArray<T>();
	}

	// Create array with enough capacity to fit file
	DynArray<T> temp;
	temp.setCapacity(static_cast<uint32_t>(size + 1));

	// Read the file into the array
	uint8_t buffer[BUFSIZ];
	size_t readSize;
	size_t currOffs = 0;
	while ((readSize = std::fread(buffer, 1, BUFSIZ, file)) > 0) {

		// Ensure array has space.
		temp.ensureCapacity(static_cast<uint32_t>(currOffs + readSize));

		// Copy chunk into array
		std::memcpy(temp.data() + currOffs, buffer, readSize);
		currOffs += readSize;
	}

	// Set size of array
	temp.setSize(static_cast<uint32_t>(currOffs));

	std::fclose(file);
	return std::move(temp);
}*/

// Exposed function
// ------------------------------------------------------------------------------------------------

bool stdSortAlgorithm(const char* filePath) noexcept
{
	return false;
}
