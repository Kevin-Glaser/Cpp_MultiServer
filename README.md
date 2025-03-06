# Cpp_MultiServer
###### 1.Learning Only
Learning only,not for commercial use
###### 2.Thrid parts:
~~libftp(https://github.com/mkulke/ftplibpp) programmed by mkulke&PhilipDeegan&wadealer&mkay229.~~
JSON for Modern C++(https://github.com/nlohmann/json) programmed by Niels Lohmann.

###### 3.Current status of this project
Based on C++17, In the implemented C++ classes, RAII,pimpl and singleton patterns are used whenever possible.
###### 4.Future Plans
In addition,Cpp_MultiServer has already started using DB connection pools,but not finished yet,it will be completed in the future.
###### 5.Notes on Certificates and Attribution
~~The libftp(https://github.com/mkulke/ftplibpp) uses LGPL2.1, any copy or commercial needs are supposed to contact the author of libftp.~~
The JSON for Modern C++(https://github.com/nlohmann/json) uses MIT license, any copy or commercial needs are supposed to contact the author of nlohmann/json.

# Build
## Build on Linux
both make and cmake are supported
#### Use Makefile
```bash
make
```

#### Use cmake
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Build On Windows
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
# or
cmake --build . --config Debug
```

## How to use
the executable files will be saved at build/bin,
```bash
cd ./build/Guardian
./Guardian
```
