CC = gcc
BINDIR = bin
OBJDIR = build
SRCDIR = src

all: main.o api.o parser.o
	$(CC) -g $(OBJDIR)/main.o $(OBJDIR)/api.o $(OBJDIR)/parser.o -o $(BINDIR)/parser

main.o: $(SRCDIR)/main.c
	$(CC) -g -c $(SRCDIR)/main.c -o $(OBJDIR)/main.o

api.o: $(SRCDIR)/api.c $(SRCDIR)/api.h
	$(CC) -g -c $(SRCDIR)/api.c -o $(OBJDIR)/api.o

parser.o: $(SRCDIR)/parser.c $(SRCDIR)/parser.h
	$(CC) -g -c $(SRCDIR)/parser.c -o $(OBJDIR)/parser.o