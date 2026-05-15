# Run with: mingw32-make (Windows) or make (Linux/macOS)
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c99 \
          -I../firmware/Core/Inc \
          -I. \
          -Istubs

SRC_DIR    = ../firmware/Core/Src
UNITY_SRC  = unity.c
STUB_SRC   = stubs/stub_freertos.c

.PHONY: all test clean

all: test

test: test_filter test_bpm
	./test_filter
	./test_bpm

test_filter: test_filter.c $(SRC_DIR)/filter.c $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test_bpm: test_bpm.c $(SRC_DIR)/bpm.c $(UNITY_SRC) $(STUB_SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test_filter test_bpm test_filter.exe test_bpm.exe
