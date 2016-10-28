// Copyright(c) Peter Hillerström(skipifzero.com, peter@hstroem.se)

#include <cstdio>
#include <chrono>
#include <iostream>
#include <vector>

#include "NaiveSmartAlgorithm.hpp"
#include "OptimizedSmartAlgorithm.hpp"
#include "OptimizedSmartAlgorithm2.hpp"
#include "OptimizedSmartAlgorithm3.hpp"
#include "StdSortAlgorithm.hpp"

// Statics
// ------------------------------------------------------------------------------------------------

using time_point = std::chrono::high_resolution_clock::time_point;

// Returns the time in milliseconds since the last call to this function
static double timeSinceLastCall(time_point& previousTime) noexcept
{
	time_point currentTime = std::chrono::high_resolution_clock::now();

	using DoubleSecond = std::chrono::duration<double, std::milli>;
	double delta = std::chrono::duration_cast<DoubleSecond>(currentTime - previousTime).count();

	previousTime = currentTime;
	return delta;
}

int main(int argc, char** argv)
{
	const size_t NUM_TESTS = 3;
	const char* TEST_FILE_PATHS[NUM_TESTS] = {
		"Rgn00.txt",
		"Rgn01.txt",
		"Rgn02.txt"
	};
	const bool TEST_FILE_CORRECT_RESULTS[NUM_TESTS] = {
		true,
		true,
		false
	};

	const size_t NUM_ALGORITHMS = 3;
	const char* ALGORITHM_NAMES[NUM_ALGORITHMS] = {
		//"StdSortAlgorithm",
		//"NaiveSmartAlgorithm",
		"OptimizedSmartAlgorithm",
		"OptimizedSmartAlgorithm2",
		"OptimizedSmartAlgorithm3"
	};
	bool(*ALGORITHMS[NUM_ALGORITHMS])(const char* path) = {
		//stdSortAlgorithm,
		//naiveSmartAlgorithm,
		optimizedSmartAlgorithm,
		optimizedSmartAlgorithm2,
		optimizedSmartAlgorithm3
	};
	
	const size_t NUM_TEST_ITERATIONS = 4;

	for (size_t algorithmIndex = 0; algorithmIndex < NUM_ALGORITHMS; algorithmIndex++) {
		
		printf("Testing algorithm: %s\n", ALGORITHM_NAMES[algorithmIndex]);
		auto algorithm = ALGORITHMS[algorithmIndex];

		double overallRuntime = 0.0;
		for (size_t testIndex = 0; testIndex < NUM_TESTS; testIndex++) {
			
			const char* testFilePath = TEST_FILE_PATHS[testIndex];
			bool correctResult = TEST_FILE_CORRECT_RESULTS[testIndex];
			double runtime = 0.0;

			for (size_t testIteration = 0; testIteration < NUM_TEST_ITERATIONS; testIteration++) {
				time_point time;
				timeSinceLastCall(time);
				bool result = algorithm(testFilePath);
				runtime += timeSinceLastCall(time);
				if (result != correctResult) {
					printf("WARNING: Algorithm \"%s\" returned incorrect result\n", testFilePath);
				}
			}

			runtime /= double(NUM_TEST_ITERATIONS);
			overallRuntime += runtime;
			printf("Test \"%s\" avg runtime: %.4f ms\n", testFilePath, runtime);
		}
		
		overallRuntime /= double(NUM_TESTS);
		printf("Total avg runtime: %.4f ms\n\n", overallRuntime);
	}
	system("pause");
}
