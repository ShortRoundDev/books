CC=gcc
SRC=./src
BIN=./bin
LIBS=-lncursesw -lm

all:
	$(CC) $(SRC)/books.c -o $(BIN)/books $(LIBS)
