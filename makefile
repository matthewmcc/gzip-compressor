all: 3thread1.0 3thread 2thread 0thread

3thread1.0: 3thread1.0.c
	gcc -Wall 3thread1.0.c -lz -lpthread -o 3thread1.0

3thread: 3thread.c 
	gcc -Wall 3thread.c -lz -lpthread -o 3thread

2thread: 2thread.c
	gcc -Wall 2thread.c -lz -lpthread -o 2thread

0thread: 0thread.c
	gcc -Wall 0thread.c -lz -lpthread -o 0thread
