cmake_minimum_required(VERSION 3.24)
project(tetris_game)

set(CMAKE_CXX_STANDARD 17)
include_directories(include /opt/homebrew/include/SDL2)
add_compile_options(-Wall)
add_library(text-game-engine
        include/TextGameEngine.hpp
        include/TextGameEngine.cpp)
target_link_libraries(text-game-engine -lSDL2 -lSDL2_ttf)
target_link_libraries(text-game-engine -L/opt/homebrew/lib/)
add_executable(tetris_game src/source.cpp)
target_link_libraries(tetris_game text-game-engine)
