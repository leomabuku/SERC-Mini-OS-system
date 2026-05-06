CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Iinclude
COMMON=src/logger.c src/process.c src/memory.c src/scheduler.c src/ipc.c src/deadlock.c src/file_manager.c src/system_state.c
RAYLIB_LIBS=-lraylib -lopengl32 -lgdi32 -lwinmm

.PHONY: all console gui tests clean

all: console gui tests

console:
	$(CC) $(CFLAGS) -o build/serc_console.exe $(COMMON) src/main.c

gui:
	$(CC) $(CFLAGS) -o build/serc_gui.exe $(COMMON) src/raylib_gui.c $(RAYLIB_LIBS)

tests:
	$(CC) $(CFLAGS) -o build/core_tests.exe $(COMMON) tests/core_tests.c

clean:
	rm -f build/serc_console.exe build/serc_gui.exe build/core_tests.exe serc_console serc_gui.exe serc_console.exe
