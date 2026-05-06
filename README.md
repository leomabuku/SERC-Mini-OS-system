# SERC Mini-OS System

SERC Mini-OS is a C-based simulator for a Smart Emergency Response Center. It models core operating-system concepts such as process control blocks, CPU scheduling, memory allocation, IPC, deadlock checking, and file/report management.

## What's included

- **Process management** with task creation, suspension, resumption, and termination
- **CPU scheduling** with FCFS, SJF, Priority Scheduling, and Round Robin
- **Scheduler comparison** with waiting time, turnaround time, CPU utilization, and Gantt data
- **Memory management** with First Fit, Best Fit, Worst Fit, and Paging
- **IPC** through a simulated message-passing flow
- **Deadlock safety checks** using Banker's Algorithm-style resource validation
- **File management** for saving and reading status snapshots and schedule reports
- **Two user interfaces**:
  - console simulator in `src/main.c`
  - raylib dashboard in `src/raylib_gui.c`

> Note: `src/gui_main.c` is a legacy Win32 GUI kept as a reference/fallback implementation.

## Repository layout

```text
serc_mini_os/
|-- build_windows.bat
|-- include/
|-- src/
|-- tests/
|-- data/
|-- logs/
|-- build/
`-- Makefile
```

## Prerequisites

### Windows
- GCC toolchain
- MSYS2 UCRT64 is recommended
- Raylib for the graphical dashboard

Install the required MSYS2 packages:

```powershell
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
```

If needed, ensure MSYS2 is on your `PATH` before building:

```powershell
$env:PATH='C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
```

## Build

From inside the `serc_mini_os` folder:

```powershell
.\build_windows.bat
```

This script builds:
- `build\serc_console.exe`
- `build\core_tests.exe`
- `build\serc_gui.exe`

You can also build with `make` in an MSYS2-compatible shell.

## Run

### Console simulator

```powershell
.\build\serc_console.exe
```

### Raylib dashboard

```powershell
.\build\serc_gui.exe
```

### Regression tests

```powershell
.\build\core_tests.exe
```

## Test coverage

The included tests verify:

- demo data loads the expected number of active tasks
- scheduler comparison does not mutate the live process table
- the saved Gantt schedule remains available after processes terminate
- paging allocates and releases frames correctly
- file-management features can save, list, and read reports

## Project notes

- Runtime reports are written to `data/`
- Logs are written to `logs/`
- Build outputs are written to `build/`

If you are viewing this repository on GitHub, this file is the main landing page for the project.


