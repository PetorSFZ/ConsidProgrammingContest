# HasDuplicates program

	Copyright
	Peter Hillerström
	peter.hillerstrom93@gmail.com
	skipifzero.com

## About

This is my entry for the Consid programming contest. It's a program that takes a list of car registration numbers (AAA000), and returns whether it contains any duplicates or not.

It is assumed that input files are correctly formatted. If they aren't the program will likely return weird values. More specifically, the following assumptions are made:

* Each row (except the last one) contains a registration number
* There MUST be a newline at end of file
* There may not be any extra whitespace or characters in the file

Nothing strange as the asumptions are supported by the contest description and provided test files. In addition it is assumed that the file uses Windows line endings (CR+LF), like all the provided test files did. Windows file endings uses 2 bytes, while Unix uses 1 byte, so some assumption regarding this needs to be made in order to process the file in binary mode. Processing in binary mode is wanted as it greatly increases speed. If for some reason Unix file endings need to be used the program can be recompiled with the constant `BYTES_PER_CODE` changed from `8` to `7`, this has not been tested however.

## Building

The program builds with CMake (version 3.0 or newer) and Visual Studio (2015 or newer). Step by step guide:

1. Create a directory called `build` inside the directory containing this readme and the source file.
2. Open a CMD prompt inside the `build` directory and enter the following command: `cmake .. -G "Visual Studio 14 2015 Win64"`
3. Open the newly generated Visual Studio solution.
4. Change build mode from `Debug` to `Release`
5. Build
