CC = gcc
CFLAGS = -lc -lm -lliquid -pthread

build:
	$(CC) $(CFLAGS) -o main main.c

backup:
	cp main.c backup.c

clean:
	rm -f main training_data.csv
