CC = gcc -g -Wall -Wextra -Werror

run: compile
	./main

compile: main.c thread/thread.c database/database.c 
	$(CC) -o main main.c thread/thread.c server.c database/database.c -lpthread

.PHONY: clean
clean: main
	rm ./main
