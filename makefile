CFLAGS=-Wall -Wextra -ggdb
INC=-I.
LIBS=-lwiringPi
default:
	g++ -c main.cpp $(CFLAGS) $(INC)
	g++ -o a.out main.o $(LIBS)
