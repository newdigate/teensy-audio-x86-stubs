cmake_minimum_required(VERSION 3.10)
project(teensy_audio_soundio_emulator_queue_example)
set(CMAKE_CXX_STANDARD 11)

find_package(teensy_x86_stubs)
include_directories(${teensy_x86_stubs_INCLUDE_DIR})

find_package(teensy_audio_x86_stubs)
include_directories(${teensy_audio_x86_stubs_INCLUDE_DIR})

find_package(teensy_x86_sd_stubs)
include_directories(${teensy_x86_sd_stubs_INCLUDE_DIR})

include_directories(/usr/local/include)
add_executable(teensy_audio_soundio_emulator_input_example
        queue.cpp
        ../../src/output_soundio.h
        ../../src/output_soundio.cpp
        ../../src/input_soundio.h
        ../../src/input_soundio.cpp)

target_link_libraries(teensy_audio_soundio_emulator_input_example ${teensy_x86_stubs_LIBS})
target_link_libraries(teensy_audio_soundio_emulator_input_example ${teensy_audio_x86_stubs_LIBS})
target_link_libraries(teensy_audio_soundio_emulator_input_example ${teensy_x86_sd_stubs_LIBS})
target_link_libraries(teensy_audio_soundio_emulator_input_example /usr/local/lib/libsoundio.a)
target_link_libraries(teensy_audio_soundio_emulator_input_example "-framework CoreServices" "-framework CoreAudio" "-framework Foundation" "-framework AudioUnit")