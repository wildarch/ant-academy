# Ant Academy

## How to Use
If you use Linux, install SFML's dependencies using your system package manager. 
On Ubuntu and other Debian-based distributions you can use the following commands:

```
sudo apt update
sudo apt install \
    libxrandr-dev \
    libxcursor-dev \
    libudev-dev \
    libfreetype-dev \
    libopenal-dev \
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev
```

Configure and build the project. 
Using CMake from the command line is straightforward:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```