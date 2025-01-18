![image](https://github.com/user-attachments/assets/94043379-2548-4468-8705-436bacb2a54a)

A simple C++ program that draws an XY oscillogram of sound from microphone.

# Build instructions
## Windows
Ensure to install [git](https://git-scm.com/), [cmake](https://cmake.org/) and [vcpkg](https://vcpkg.io/en/), then run these commands:
```
git clone https://github.com/Smok1e/Oscilloscope
cd Oscilloscope
git submodule init && git submodule update
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Linux
Ensure to install [git](https://git-scm.com/), [cmake](https://cmake.org/) and all the sfml dependencies listed [here](https://www.sfml-dev.org/tutorials/2.6/compile-with-cmake.php#installing-dependencies), then run these commands:
```
git clone https://github.com/Smok1e/Oscilloscope
cd Oscilloscope
git submodule init && git submodule update
mkdir build && cd build
cmake ..
cmake --build .
```
