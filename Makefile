#Files to compile
OBJS = tetris.cpp

CC = g++

INCLUDE_PATHS = -I.\SDL2-2.0.7\x86_64-w64-mingw32\include\SDL2 -I.\SDL2_ttf-2.0.14\x86_64-w64-mingw32\include\SDL2
LIBRARY_PATHS = -L.\SDL2-2.0.7\x86_64-w64-mingw32\lib -L.\SDL2_ttf-2.0.14\x86_64-w64-mingw32\lib

#LIBRARY_PATHS = -LC:\mingw32_dev_lib\lib

# -w supresses warnings
# -Wl,-subsystem,windows removes the console window
#  -s will run strip, reducing the size of the executable
#COMPILER_FLAGS = -w -Wl,-subsystem,windows
COMPILER_FLAGS = -std=c++14 -Wall -Wpedantic -s -O3 -Wl,-subsystem,windows

# -g: Debugging symbols -pg profiling
#COMPILER_FLAGS = -std=c++14 -Wall -Wpedantic -g

LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf


# Targets
tetris: $(OBJS)
	$(CC) tetris.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o .\bin\tetris.exe

