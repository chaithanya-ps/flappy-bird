CC = gcc
LIBS = -lncurses -lm

flappy_bird: Flappy_Bird_Project.c
	$(CC) -o game Flappy_Bird_Project.c $(LIBS)

clean:
	rm -f flappy_bird
