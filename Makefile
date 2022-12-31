#OBJS specifies which files to compile as part of the project
OBJS = src/source.cpp

#CC specifies which compiler we're using
CC = clang++

#COMPILER_FLAGS specifies the additional compilation options we're using
# replace the flags with sdl2-config --cflags --libs when releasing
COMPILER_FLAGS = -Wall -std=c++17 -g -Iinclude -I/opt/homebrew/include/SDL2

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = bin/debug/main

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
