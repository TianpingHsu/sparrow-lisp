
CC = gcc
CFLAGS = -g -Wall
SRC = sparrow.c

sparrow: $(SRC)
	gcc $(CFLAGS) $^ -o $@


test: $(SRC)
	gcc $(CFLAGS) -D DEBUG  $^ -o $@
	./test

clean:
	rm -rf sparrow test $(OBJS)

