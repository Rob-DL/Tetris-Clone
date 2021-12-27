cmake_minimum_required(VERSION 3.10)

project(Tetris-Clone)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(C:\\lib\\SDL2-2.0.18\\include C:\\lib\\SDL2_ttf-2.0.15\\include)
link_directories(C:\\lib\\SDL2-2.0.18\\lib\\x64 C:\\lib\\SDL2_ttf-2.0.15\\lib\\x64)

add_executable(tetris tetris.cpp)

target_link_libraries(tetris SDL2main SDL2 SDL2_ttf)


