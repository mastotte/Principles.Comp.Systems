CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic

all: queue.o

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f queue queue.o

format:
	clang-format -i -style=file queue.c queue.h