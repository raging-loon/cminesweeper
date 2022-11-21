all: main
CC=gcc

main:
	$(CC) cminesweeper.c -o cminesweeper -lcurses -ggdb 
