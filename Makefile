
CC = gcc
CFLAGS = -g -Wall -std=gnu11
SRC = sparrow.c

sparrow: $(SRC)
	gcc $(CFLAGS) $^ -o $@


test: $(SRC)
	gcc $(CFLAGS) -D DEBUG  $^ -o $@
	@./test

mceval: $(SRC)
	gcc $(CFLAGS) -D META_EVAL $^ -o $@
	@./mceval


clean:
	rm -rf sparrow test mceval $(OBJS)

