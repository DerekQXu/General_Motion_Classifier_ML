CC = gcc
CFLAGS = -lc -lm -lliquid -pthread

build:
	$(CC) $(CFLAGS) -o main main.c

clean:
	rm -f main 
