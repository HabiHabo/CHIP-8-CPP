cmake_minimum_required(VERSION 3.10.0)
project(chip8_cpp VERSION 0.1.0 LANGUAGES C CXX)

add_executable(chip8_cpp src/main.cpp)

target_link_libraries(chip8_cpp SDL2main SDL2)

