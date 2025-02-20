cmake_minimum_required(VERSION 3.10)
project(teensy_audio_soundio_emulator)
set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/include)
add_library(teensy_audio_soundio_emulator
        src/output_soundio.h
        src/output_soundio.cpp)
