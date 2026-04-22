# Ncurses Flappy Bird

**Prerequisites:**  
  gcc  
  ncurses library

**Compilation on Linux**
```bash
gcc Flappy_Bird_Project.c -lncurses -o game
```

If lncurses is not installed, it can be installed using the command
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```
To **run** the game:
```bash
./game
```
About the game:

This is a remake of the very infamous and old video game FLAPPY BIRD.  
This code can only be run on a Linux Terminal as it uses the terminal as the interface and code is Linux specific.  

To play the game, either download and extract the compressed zip directly from the GitHub page, or `gh repo clone chaithanya-ps/flappy-bird` if using GitHub terminal client, or clone [https://github.com/chaithanya-ps/flappy-bird](https://github.com/chaithanya-ps/flappy-bird)  
The instructions to play the game are very simple and clearly mentioned at the start of the game.


A retro, terminal-based Flappy Bird clone written in C using the **ncurses** library. Experience different physics across the solar system with Earth, Moon, and Jupiter gravity modes!

## Features

* **Multi-Planet Modes:** Choose between Earth (Normal), Moon (Low), and Jupiter (High) gravity modes.
* **Difficulty Levels:** Easy, Medium, and Hard modes adjust the pipe gaps and initial speeds.
* **Persistent High Scores:** Your best scores for every combination of Mode and Difficulty are saved to `high_scores.txt`.
* **Dynamic Leveling:** The game speeds up as your score increases, with "LEVEL UP" notifications.
* **Colorized UI:** Uses ncurses color pairs for birds, pipes, and background elements.


