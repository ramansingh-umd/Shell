CC = gcc
CFLAGS = -ansi -Wall -g -O0 -Wwrite-strings -Wshadow \
	 -pedantic-errors -fstack-protector-all -Wextra

PROGS = d8sh

all: $(PROGS)

executor.o: executor.c executor.h command.h
	$(CC) $(CFLAGS) -c executor.c

d8sh: d8sh.o executor.o lexer.o parser.tab.o
	$(CC) -o d8sh d8sh.o executor.o lexer.o parser.tab.o -lreadline

d8sh.o: d8sh.c executor.h lexer.h parser.tab.h 
	$(CC) $(CFLAGS) -c d8sh.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

parser.tab.o: parser.tab.c parser.tab.h
	$(CC) $(CFLAGS) -c parser.tab.c

clean:
	rm -f *.o $(PROGS) a.out
