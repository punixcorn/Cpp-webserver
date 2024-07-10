CC=g++
FILE=main.cpp
STD=-std=c++2a
FLAGS=-g --all-warnings

all:
	cmake -S . -B build 
	cmake --build build

run: all
	@echo "========================================="
	@./build/webserver

clean:
	@touch main.o main a.out 
	@rm -rf *.o main a.out build
