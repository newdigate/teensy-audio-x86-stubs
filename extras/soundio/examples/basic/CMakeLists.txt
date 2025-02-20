cmake_minimum_required(VERSION 3.10)
project(teensy_audio_soundio_emulator_basic_example)
set(CMAKE_CXX_STANDARD 11)

include_directories(../../../../src)

include_directories(/usr/local/include)
add_executable(teensy_audio_soundio_emulator_basic_example
        basic.cpp
        ../../src/output_soundio.h
        ../../src/output_soundio.cpp)

target_link_libraries(teensy_audio_soundio_emulator_basic_example teensy_x86_stubs)
target_link_libraries(teensy_audio_soundio_emulator_basic_example teensy_x86_sd_stubs)
target_link_libraries(teensy_audio_soundio_emulator_basic_example teensy_audio_x86_stubs)
target_link_libraries(teensy_audio_soundio_emulator_basic_example "-L/usr/local/lib -lsoundio")
if  (APPLE)
        target_link_libraries(teensy_audio_soundio_emulator_basic_example
                "-framework CoreServices"
                "-framework CoreAudio"
                "-framework Foundation"
                "-framework AudioUnit")
endif()