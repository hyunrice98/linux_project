smallsh: main.o smallsh.o smallshHeader.o
	gcc -o smallsh main.o smallsh.o smallshHeader.o

main.o: main.c
	gcc -c -o main.o main.c

smallsh.o: smallsh.c
	gcc -c -o smallsh.o smallsh.c

smallshHeader.o: smallsh.h
	gcc -c -o smallshHeader.o smallsh.h

clean:
	rm *.o smallsh