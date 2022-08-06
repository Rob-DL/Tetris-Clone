cmake_minimum_required(VERSION 3.10)

project(Tetris-Clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(/usr/include/SDL2)
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(tetris tetris.cpp)

target_link_libraries(tetris SDL2main SDL2 SDL2_ttf)


