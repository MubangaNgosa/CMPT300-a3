# Define what compiler to use and the flags.
# build an executable named lets_talk from lets_talk.c
all: lets-talk.c
	gcc -g -Wall -pthread -o lets-talk lets-talk.c list.c

clean:
	$(RM) lets-talk

valgrind:
	valgrind --leak-check=full ./lets-talk 3000 localhost 3001