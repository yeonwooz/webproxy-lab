CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder tester

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

tester: tester.c
	$(CC) $(CFLAGS) -o tester tester.c

clean:
	rm -f adder *~
	rm -f tester *~
