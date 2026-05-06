@echo off
setlocal

if not exist logs mkdir logs
if not exist build mkdir build

set "GCC=gcc"
where gcc >nul 2>nul
if errorlevel 1 (
    if exist C:\msys64\ucrt64\bin\gcc.exe (
        set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
        set "GCC=C:\msys64\ucrt64\bin\gcc.exe"
    )
)

echo Building SERC Mini-OS console application...
"%GCC%" -std=c11 -Wall -Wextra -Iinclude ^
src\logger.c ^
src\process.c ^
src\memory.c ^
src\scheduler.c ^
src\ipc.c ^
src\deadlock.c ^
src\file_manager.c ^
src\system_state.c ^
src\main.c ^
-o build\serc_console.exe

if errorlevel 1 (
    echo Console build failed.
    exit /b 1
)

echo Building SERC Mini-OS tests...
"%GCC%" -std=c11 -Wall -Wextra -Iinclude ^
src\logger.c ^
src\process.c ^
src\memory.c ^
src\scheduler.c ^
src\ipc.c ^
src\deadlock.c ^
src\file_manager.c ^
src\system_state.c ^
tests\core_tests.c ^
-o build\core_tests.exe

if errorlevel 1 (
    echo Test build failed.
    exit /b 1
)

if not exist C:\msys64\ucrt64\include\raylib.h (
    echo Raylib headers not found.
    echo Install with: pacman -S --needed mingw-w64-ucrt-x86_64-raylib
    exit /b 1
)

echo Building SERC Mini-OS raylib dashboard...
"%GCC%" -std=c11 -Wall -Wextra -Iinclude ^
src\logger.c ^
src\process.c ^
src\memory.c ^
src\scheduler.c ^
src\ipc.c ^
src\deadlock.c ^
src\file_manager.c ^
src\system_state.c ^
src\raylib_gui.c ^
-o build\serc_gui.exe ^
-lraylib -lopengl32 -lgdi32 -lwinmm

if errorlevel 1 (
    echo GUI build failed.
    exit /b 1
)

echo Build successful.
echo Console EXE: build\serc_console.exe
echo Tests EXE: build\core_tests.exe
echo Raylib GUI EXE: build\serc_gui.exe

endlocal
pause
