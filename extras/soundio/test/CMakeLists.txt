cmake_minimum_required(VERSION 3.10)
project(soundio_test_sketch)
set(CMAKE_CXX_STANDARD 11)

DeclareAndFetch(teensy_x86_sd_stubs             git@github.com:newdigate/teensy-x86-sd-stubs noinstall src)

add_executable(main test.cpp)

target_link_libraries(main /usr/local/lib/libsoundio.a)
target_link_libraries(main "-framework CoreServices" "-framework CoreAudio" "-framework Foundation" "-framework AudioUnit")