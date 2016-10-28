# Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project("ConsidProgrammingContest")

# Compiler flags
if(MSVC)
	if((MSVC_VERSION LESSER 1800) OR (MSVC_VERSION EQUAL 1800))
		message(FATAL_ERROR "Too old version of Visual Studio, 2015 or newer required.")
	endif()
	set(CMAKE_CXX_FLAGS "/W3 /Zi /EHsc /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2")
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")
else()
	message(FATAL_ERROR "Non-MSVC support not yet implemented")
endif()

# Executable
add_executable(ConsidProgram
	${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/NaiveSmartAlgorithm.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/NaiveSmartAlgorithm.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/OptimizedSmartAlgorithm.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/OptimizedSmartAlgorithm.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/OptimizedSmartAlgorithm2.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/OptimizedSmartAlgorithm2.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/StdSortAlgorithm.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/StdSortAlgorithm.cpp
)

# Copy test files to binary dir
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_files/Rgn00.txt DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_files/Rgn01.txt DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_files/Rgn02.txt DESTINATION ${CMAKE_BINARY_DIR})
