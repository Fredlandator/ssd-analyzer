@echo off
echo Building SSD Analyzer Plugin for Saleae Logic 2...

if not exist build (
    mkdir build
)

cd build
cmake -A x64 ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Plugin files are in: build\Analyzers\Release\
echo Copy the .dll file to your Saleae Logic 2 analyzer plugins directory.
echo.
pause