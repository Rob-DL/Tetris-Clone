#Files to compile
OBJS = ../src/tetris.cpp

CC = g++

INCLUDE_PATHS = -I/usr/include/SDL2/ -I/usr/include/SDL2_ttf
LIBRARY_PATHS = -L/usr/lib/

# -w supresses warnings
# -Wl,-subsystem,windows removes the console window
#  -s will run strip, reducing the size of the executable
#COMPILER_FLAGS = -w -Wl,-subsystem,windows
COMPILER_FLAGS = -std=c++14 -Wall -Wpedantic -s -O3

# -g: Debugging symbols -pg profiling
#COMPILER_FLAGS = -std=c++14 -Wall -Wpedantic -g

LINKER_FLAGS = -lSDL2main -lSDL2 -lSDL2_ttf


# Targets
tetris: $(OBJS)
	$(CC) ../src/tetris.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o ../bin/tetris
	cp ../src/consola.ttf ../bin/
