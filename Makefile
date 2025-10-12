all: run

main.o: src/main.c
	gcc -Wall -g -c src/main.c -o ./build/main.o

main: main.o
	gcc ./build/main.o -o ./build/main

run: main
	./build/main

clean:
	rm -f build/main.o build/main

rebuild: clean all
