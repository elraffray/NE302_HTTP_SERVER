CC = gcc

all: main.o api.o parser.o
	$(CC) -g main.o api.o parser.o -o parser

main.o: main.c
	$(CC) -g -c main.c

api.o: api.c api.h
	$(CC) -g -c api.c

parser.o: parser.c parser.h
	$(CC) -g -c parser.c