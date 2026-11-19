# Makefile for cz1dump (Casio CZ-1 SysEx Preset Dump CUI Tool)
# Compatible with MSYS2 / MinGW-w64 on Windows, Linux, and macOS

CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
LDFLAGS ?= -static -static-libgcc -static-libstdc++

TARGET = cz1dump
ifeq ($(OS),Windows_NT)
    TARGET = cz1dump.exe
endif

SRC = cz1dump.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) cz1dump.exe cz1dump

.PHONY: all clean
