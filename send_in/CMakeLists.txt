# Copyright (c) Peter Hillerström (peter.hillerstrom93@gmail.com, skipifzero.com)

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project("HasDuplicates")

# Require Visual Studio 2015 or newer
if((MSVC_VERSION LESSER 1800) OR (MSVC_VERSION EQUAL 1800))
	message(FATAL_ERROR "Too old version of Visual Studio, 2015 or newer required.")
endif()

# Require 64bit compiler
if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 8))
	message(FATAL_ERROR "Can only compile in 64bit mode")
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "/W3 /Zi /EHsc /D_CRT_SECURE_NO_WARNINGS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/O2")
set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG")

# Executable
add_executable(HasDuplicates ${CMAKE_CURRENT_SOURCE_DIR}/HasDuplicates.cpp)
