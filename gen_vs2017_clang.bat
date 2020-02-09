@echo off
mkdir build64
cd build64
cmake .. -G "Visual Studio 15 2017 Win64" -T "llvm"
cmake --build . --config Release
cd ..
pause