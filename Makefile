CC = gcc
CFLAGS = -lc -lm -lliquid -pthread

build: clean
	$(CC) $(CFLAGS) -o main main.c

clean:
	rm -f main training_data.csv
