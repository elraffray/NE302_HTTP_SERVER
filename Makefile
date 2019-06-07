CC = gcc
BINDIR = bin
OBJDIR = build
SRCDIR = src
LIBS = -L./lib -lrequest

parser: main.o api.o parser.o semantic.o fcgiHandler.o
	$(CC) -g -Wall $(LIBS) $(OBJDIR)/main.o $(OBJDIR)/api.o $(OBJDIR)/semantic.o $(OBJDIR)/parser.o $(OBJDIR)/fcgiHandler.o -o $(BINDIR)/parser

semantic.o: $(SRCDIR)/semantic.c $(SRCDIR)/semantic.h
	$(CC) -g -Wall $(LIBS) -c $(SRCDIR)/semantic.c -o $(OBJDIR)/semantic.o

main.o: $(SRCDIR)/main.c
	$(CC) -g -Wall $(LIBS) -c $(SRCDIR)/main.c -o $(OBJDIR)/main.o

api.o: $(SRCDIR)/api.c $(SRCDIR)/api.h
	$(CC) -g -Wall $(LIBS) -c $(SRCDIR)/api.c -o $(OBJDIR)/api.o

parser.o: $(SRCDIR)/parser.c $(SRCDIR)/parser.h
	$(CC) -g -Wall $(LIBS) -c $(SRCDIR)/parser.c -o $(OBJDIR)/parser.o

fcgiHandler.o: $(SRCDIR)/fcgiHandler.c $(SRCDIR)/fcgiHandler.h
	$(CC) -g -Wall $(LIBS) -c $(SRCDIR)/fcgiHandler.c -o $(OBJDIR)/fcgiHandler.o