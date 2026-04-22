CC = gcc
CFLAGS = -Wall -O2
LIBS = -lncurses -lm

flappy_bird: Flappy_Bird_Project.c
	$(CC) $(CFLAGS) -o flappy_bird Flappy_Bird_Project.c $(LIBS)

clean:
	rm -f flappy_bird
