all: prodcon
prodcon: prodcon.o tands.o helper.o
	gcc -o  prodcon prodcon.o -pthread tands.o helper.o
prodcon.o: prodcon.c
	gcc -Wall -c -pthread prodcon.c -o prodcon.o
tands.o: tands.c tands.h
	gcc -Wall -c tands.c -o tands.o
helper.o: helper.c helper.h
	gcc -Wall -c helper.c -o helper.o

clean:
	rm *.o prodcon
