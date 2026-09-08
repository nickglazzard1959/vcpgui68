#!/bin/bash
#
# Build helper programs for VCPGUI components.
#
# Check CLI11.hpp is present.
if [ ! -f CLI11.hpp ]; then
    echo "CLI11.hpp not found. This must be installed to build."
    echo "Get it from https://github.com/CLIUtils/CLI11/releases"
    exit 1
fi
#
# DSURF. If there appears to be a local SDL2 build, use that preferentially.
echo "Build DSURF."
if [ -f /usr/local/lib/libSDL2.so ]; then
    echo "... using local SDL2 build."
    g++ -O0 -g -Wall dsurf.cpp -o dsurf -I/usr/local/include/SDL2 -L/usr/local/lib \
        -lSDL2 -lSDL2_gfx -lSDL2_ttf -lSDL2_image -Wl,-rpath,/usr/local/lib
else
    echo "... using system SDL2."
    g++ -O0 -g -Wall dsurf.cpp -o dsurf -lSDL2 -lSDL2_gfx -lSDL2_ttf -lSDL2_image
fi
#
# SERIALIO,
echo "Build SERIALIO."
g++ -O0 -g -Wall -o serialio serialio.cpp
#
# TMCUSBIO.
echo "Build TMCUSBIO."
g++ -O0 -g -Wall -o tmcusbio tmcusbio.cpp -lusb-1.0
#
echo "Done."
