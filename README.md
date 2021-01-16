# 2D_Editor - Using rasterization, SDL, Eigen, C++ libraries

This is a 2D editor that uses rasterization technique to draw triangles. 
It allows the below:
1. Draw triangles
2. Color the triangles
3. Translate, rotate, scale the triangles by simply click & move
4. Zoom in/out and pan left/right of the viewframe 
5. Generate, record and play animations

Please watch "2D_Editor_Demo.mp4" to watch the working demo of the editor. 

Please read below for the setup.

Setup to run:
1. Install CMake on your computer
    Linus: sudo apt-get install cmake
    Mac with HomeBrew: brew install cmake
    Windows with Chocolatey: choco install -y cmake.
2. Install a C++ compiler
    Linux: gcc/clang 
    Mac: clang 
    Windows: Visual Studio
3. Compiling the Sample Projects
    1. Download the code
    2. Create a directory called build in the repo directory
        mkdir build
    3. Use CMake to generate the Makefile/project files needed for compilation inside the build/ directory:
        cd build; cmake ..
    4. Compile and run the compiled executable by typing:
        make; 
        
Credits to Prof. Daniel Panozzo for the base project and support during the course.
