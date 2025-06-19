CC = gcc -g -Wall -Wextra -Werror

run: compile
	./main

compile: main.c server.c leader.h backup.h client.h
	$(CC) -o main main.c server.c leader.h

.PHONY: clean
clean: main
	rm ./main
