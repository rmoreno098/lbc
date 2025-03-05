# all: main

main: main.o
	gcc main.o -o main

main.o: main.c
	gcc -Wall -g -c main.c -o main.o

clean:
	rm -f main.o main

rebuild: clean all
