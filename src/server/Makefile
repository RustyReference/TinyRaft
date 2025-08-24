CC = gcc -g -Wall -Wextra -Werror

run: compile
	./main

compile: main.c thread/thread.c 
	$(CC) -o main main.c thread/thread.c server.c

backup: backup.c server.c
	$(CC) -o backup backup.c thread/thread.c server.c

client: client.c server.c
	$(CC) -o client client.c thread/thread.c server.c

.PHONY: clean
clean: main
	rm ./main
	rm ./client
	rm ./backup
