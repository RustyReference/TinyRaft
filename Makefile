CC = gcc -g -Wall -Werror

run: compile
	./main

compile: main.c kv_store.c 
	$(CC) -o main main.c kv_store.c

.PHONY: clean
clean:
	rm main
