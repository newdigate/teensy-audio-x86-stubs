cmake_minimum_required(VERSION 3.5)
project(teensy_audio_x86_stubs_soundio C CXX)
set(teensy_audio_x86_stubs_soundio_VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES
        input_soundio.cpp
        output_soundio.cpp)

set(HEADER_FILES
        input_soundio.h
        output_soundio.h)

include(CMakeLists.cmake.in)

#set(CMAKE_VERBOSE_MAKEFILE  1)
