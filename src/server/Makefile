CC = gcc -g -Wall -Wextra -Werror

run: compile
	./main

compile: main.c thread/thread.c 
	$(CC) -o main main.c thread/thread.c server.c

.PHONY: clean
clean: main
	rm ./main
