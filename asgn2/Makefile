CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic

all: httpserver

httpserver: httpserver.o
	$(CC) -o httpserver httpserver.o asgn2_helper_funcs.a

httpserver.o: httpserver.c httpserver.h
	$(CC) $(CFLAGS) -c httpserver.c

clean:
	rm -f httpserver.o httpserver
	
format:
	clang-format -i -style=file httpserver.c httpserver.h

