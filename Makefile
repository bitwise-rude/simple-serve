CC = gcc

main: main.o 
	$(CC) -o main main.o

main.o: main.c main.h
	$(CC) -c main.c 

clean:
	rm -f *.o main .main.c.swp .main.h.swp


