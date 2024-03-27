all: homework main

homework: homework.c
	@mpicc -o homework homework.c -Wall -O3

main: main.c
	@gcc main.c -o main