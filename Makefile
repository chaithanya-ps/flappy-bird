CC = gcc
LIBS = -lncurses -lm

game: Flappy_Bird_Project.c
	$(CC) -o game Flappy_Bird_Project.c $(LIBS)

clean:
	rm -f game
