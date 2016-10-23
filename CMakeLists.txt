# Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project("ConsidProgrammingContest")

# Compiler flags
if(MSVC)
	if((MSVC_VERSION LESSER 1800) OR (MSVC_VERSION EQUAL 1800))
		message(FATAL_ERROR "Too old version of Visual Studio, 2015 or newer required.")
	endif()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "/W3 /Zi /EHsc /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} "/O2 /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} "/O2")
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} "/Od /DEBUG")
else()
	message(FATAL_ERROR "Non-MSVC support not yet implemented")
endif()

add_executable(ConsidProgram
	${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
)
