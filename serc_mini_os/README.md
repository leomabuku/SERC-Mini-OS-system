# SERC Mini-OS (CS225 Assignment)

This project implements a C-based Mini Operating System simulator for a Smart Emergency Response Center.

## Components implemented
- Process Management with PCB state tracking.
- CPU Scheduling: FCFS, SJF, Priority Scheduling, and Round Robin.
- Scheduler comparison with waiting time, turnaround time, completion time, CPU utilization, and persistent Gantt data.
- Memory Management: First Fit, Best Fit, Worst Fit, and Paging bonus.
- IPC through a simulated C message queue.
- Deadlock Handling with Banker's Algorithm.
- File Management for logs, status snapshots, schedule reports, and saved local records.

## Project structure
- `include/` header files
- `src/` C source files
- `src/main.c` console menu interface
- `src/raylib_gui.c` animated raylib dashboard
- `src/gui_main.c` old Win32 GUI kept only as fallback reference
- `tests/` C regression tests
- `logs/` runtime logs
- `data/` saved reports and file-management output

## Windows setup
Install MSYS2 UCRT packages:

```powershell
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
```

Make sure this path is available before building:

```powershell
$env:PATH='C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
```

## Build
From the project folder:

```powershell
.\build_windows.bat
```

Outputs:
- `build\serc_console.exe`
- `build\serc_gui.exe`
- `build\core_tests.exe`

## Run
Console:

```powershell
.\build\serc_console.exe
```

Animated raylib dashboard:

```powershell
.\build\serc_gui.exe
```

## Raylib dashboard features
- Dark emergency-control-room visual theme.
- Dashboard cards for process counts, memory, resources, and scheduler state.
- Task creation form with service and memory strategy selection.
- Scheduling controls for FCFS, SJF, Priority, Round Robin, Compare, Summary, Computation, and Replay.
- Persistent animated Gantt chart that remains visible after scheduled processes terminate.
- Computation table with `PID`, `Burst`, `Start`, `Completion`, `Turnaround`, and `Waiting`.
- Comparison table with `Algorithm`, `Tasks`, `Avg Waiting`, `Avg Turnaround`, `CPU %`, `Time`, `Segments`, and `Best`.
- Memory screen with contiguous allocation map and paging frame grid.
- IPC screen with process nodes, queue table, and animated message passing.
- Resource/deadlock screen with resource bars and Banker's Algorithm status.
- Files/logs screen for logs, saving reports, listing files, and previewing saved data.

## Tests
Run:

```powershell
.\build\core_tests.exe
```

Covered scenarios:
- Demo loads 12 active tasks.
- Scheduler comparison does not mutate active processes.
- Persistent Gantt result survives process termination.
- Paging allocates/releases frames and tracks internal fragmentation.
- File management writes, lists, and reads saved reports.
