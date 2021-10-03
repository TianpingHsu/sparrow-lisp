
CC = gcc
CFLAGS = -g -Wall
OBJS = sparrow.o

sparrow: $(OBJS)
	gcc -g -Wall $^ -o $@


%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf sparrow $(OBJS)

