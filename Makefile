CC = gcc
CFLAGS = -lc -lm -lliquid -pthread

build:
	$(CC) $(CFLAGS) -o main main.c

setup:
	-@chmod +x *.py *.pyc *.sh *.exp main -f || true
	-@chmod +rw *.txt *.csv -f || true

clean:
	rm -f main training_data.csv custom.pyc
