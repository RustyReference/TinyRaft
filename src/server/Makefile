CC = gcc -g -w

run: compile
	./main

compile: main.c thread/thread.c database/database.c server.c 
	$(CC) -o main $^ -lpthread

backup: backup.c server.c database/database.c thread/thread.c
	$(CC) -o backup $^

client: client.c server.c database/database.c thread/thread.c
	$(CC) -o client $^

.PHONY: clean
clean: main
	rm ./main
	rm ./client
	rm ./backup
