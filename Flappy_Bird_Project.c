#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#define WIDTH 80
#define HEIGHT 24
#define PIPE_W 5
#define MAX_PIPES 5
#define GAP_EASY 10
#define GAP_MEDIUM 8
#define GAP_HARD 6
#define MAPS 3
#define BIRD_CHAR '@'
#define PIPE_CHAR 'X'
#define BG_CHAR ' '

#define COL_BIRD 1
#define COL_PIPE1 2
#define COL_PIPE2 3
#define COL_UI 4
#define COL_DANGER 5
#define COL_TITLE 6
#define COL_BACKGROUND 7

typedef enum{
    DIFF_EASY = 0,
    DIFF_MED = 1,
    DIFF_HARD = 2,
}Difficulty;

typedef enum{
    PLAY = 0,
    PAUSE = 1,
    FINISH = 2
}State;

typedef enum{
    EARTH = 0,
    MOON = 1,
    JUPITER = 2
}Mode;

typedef struct{
    float x;
    int gap;
    int passed;
    int col;
}Pipe;

typedef struct{
    float y;
    float speed;
}Bird;

typedef struct{
    const char * name;
    const char * desc;
    float gravity;
    float jump_speed;
    //int curr_hs;
}Mode_Properties;

static  Mode_Properties const modes[MAPS] ={
    {"Earth", "Normal Gravity", 0.20, -1.2},
    {"Moon", "Low Gravity", 0.09, -0.75},
    {"Jupiter", "High Gravity", 0.30, -1.6},
};


typedef struct{
    Bird bird;
    Pipe pipe[MAX_PIPES];
    int score;
    int high_score[MAPS][3];
    int level;
    Mode mode;
    State state;
    Difficulty diff;
    int frame;
    float pipe_speed;
    int gap;
    char message[100];
    int msg_timer;
}Game;

//load high score always

static void load_high_score(int hs[MAPS][3]){
    for(int i = 0; i < MAPS; i++){
        for(int j = 0; j<3; j++)
            hs[i][j] = 0;
    }

    FILE *f = fopen("high_scores.txt", "r");
    if(f){
        for(int i = 0; i < MAPS; i++){
            for(int j = 0; j<3; j++)
                fscanf(f, "%d", &hs[i][j]);
        }
        fclose(f);
    }
}

//update high score always

static void update_high_score(int hs[MAPS][3]){
    FILE *f = fopen("high_scores.txt", "w");
    if (f){
        for (int i = 0; i < MAPS; i++){
            for (int j = 0; j < 3; j++)
                fprintf(f, "%d ", hs[i][j]);
            fprintf(f, "\n");
        }
        fclose(f);
    }
}

static void jump(){
    beep();
}

static void score(){
    beep();
}

static void die(){
    for(int i = 0; i < 3; i++){
        beep();
        usleep(50000);
    }
}

static void colors(){
    if(!has_colors())
        return;
    start_color();
    use_default_colors();
    init_pair(COL_BIRD, COLOR_YELLOW, -1);
    init_pair(COL_PIPE1, COLOR_GREEN, -1);
    init_pair(COL_PIPE2, COLOR_WHITE, -1);
    init_pair(COL_DANGER, COLOR_RED, -1);
    init_pair(COL_BACKGROUND, COLOR_MAGENTA, -1);
    init_pair(COL_UI, COLOR_CYAN, -1);
    init_pair(COL_TITLE, COLOR_YELLOW, -1);
}

//initialize the game with bird and pipes

static void init_game(Game *game){
    game->bird.y = HEIGHT/2.0;
    game->bird.speed = 0.0f;
    game->score = 0;
    game->level = 1;
    game->state = PLAY;
    game->frame = 0;
    game->message[0] = '\0';
    game->msg_timer = 0;

    switch (game->diff){
        case DIFF_EASY:
            game->gap = GAP_EASY;
            game->pipe_speed = 1.0f;
            break;
        case DIFF_MED:
            game->gap = GAP_MEDIUM;
            game->pipe_speed = 1.2f;
            break;
        case DIFF_HARD:
            game->gap = GAP_HARD;
            game->pipe_speed= 1.5f;
            break;     
    }
    for(int i = 0; i < MAX_PIPES; i++){
        game->pipe[i].x = (WIDTH + i * 22);
        game->pipe[i].gap = rand() % (HEIGHT - game->gap - 4) + 2;
        game->pipe[i].passed = 0;
        game->pipe[i].col = rand()%2 + COL_PIPE1;
    }
}

//any message should get printed during game

static void msg(Game *game, char *msg, int dur){
    strcpy(game->message, msg);
    game->msg_timer = dur;
}

//update the screen everytime to move the pipes

static void update(Game *game){
    if(game->state != PLAY)
        return;
    game->frame++;
    if(game->msg_timer > 0)
        game->msg_timer--;
    if(game->msg_timer == 0)
        game->message[0] = '\0';
    game->bird.speed += modes[game->mode].gravity;
    game->bird.y += game->bird.speed;
    
    if(game->bird.y < 0 || game->bird.y >= HEIGHT){
        game->state = FINISH;
        die();
        return;
    }

    float speed = game->pipe_speed + (game->level - 1)*0.05; //increase speed on levlling up

    for(int i = 0; i < MAX_PIPES; i++){
        game->pipe[i].x -= speed;
        if(game->pipe[i].x <= 10 && game->pipe[i].x + PIPE_W >=10){
            int y = game->bird.y;
            if(y < game->pipe[i].gap || y > game->pipe[i].gap + game->gap){
                game->state = FINISH;
                die();
                return;
            }
        }

        if(!game->pipe[i].passed && game->pipe[i].x + PIPE_W <10){
            game->pipe[i].passed++;
            game->score++;
            score();
        }
        int new_level = (game->score / 10) + 1;
        if (new_level > game->level){
            game->level = new_level;
            msg(game, "LEVEL UP!", 40);
        }
        if (game->pipe[i].x + PIPE_W < 0){
                float max_x = 0;
                for (int j = 0; j < MAX_PIPES; j++)
                    if (game->pipe[j].x > max_x) max_x = game->pipe[j].x;
                int interval = (WIDTH - 15) / MAX_PIPES;
                if (interval < 14) interval = 14;
                game->pipe[i].x = max_x + interval;
                game->pipe[i].gap = rand() % (HEIGHT - game->gap - 4) + 2;
                game->pipe[i].passed = 0;
                game->pipe[i].col = rand() % 2 + COL_PIPE1;
            }
        }

        if (game->score > game->high_score[game->mode][game->diff])
            game->high_score[game->mode][game->diff] = game->score;

}

static void draw_border(){
    for(int i = 0; i < WIDTH; i++){
        mvaddch(0, i, '-');
        mvaddch(HEIGHT-1, i, '-');
    }
}

//draw background for moon and jupiter

static void draw_bg(Game *game){
    switch (game->mode){
        case MOON:
            attron(COLOR_PAIR(COL_BACKGROUND));
            for(int i = 0; i<WIDTH; i++){
                if(i % 8 == 0)
                    mvaddch(1, i, 'o');
                if(i % 5 == 0)
                    mvaddch(2, i, 'o');
            }
            break;
        case JUPITER:
            attron(COLOR_PAIR(COL_BACKGROUND));
            for(int i = 0; i < WIDTH; i++){
                mvaddch(4, i, '-');
                mvaddch(9, i, '-');
                mvaddch(14, i, '-');
                mvaddch(19, i, '-');
            }
            break;
    }
    attroff(COLOR_PAIR(COL_BACKGROUND));
}

//draw pipes and replace it with a new one after it has goes out of the screen

static void draw_pipes(Game *game){
    for(int i = 0; i < MAX_PIPES; i++){
        int px = (int)game->pipe[i].x;
        int color = game->pipe[i].col;
        attron(COLOR_PAIR(color));
        for (int x = 0; x < PIPE_W; x++){
            int col = px + x;
            if(col < 0 || col >= WIDTH)
                continue;
            else{
                for(int y = 1; y < HEIGHT - 1; y++){
                    if(y < game->pipe[i].gap || y > game->pipe[i].gap + game->gap)
                        mvaddch(y, col, PIPE_CHAR);
                }
            }
        }
        attroff(COLOR_PAIR(color));
    }
}

//draw bird 

static void draw_bird(Game *game){
    int y = game->bird.y;
    if(y < 1 || y >= HEIGHT){
        game->state = FINISH;
        die();
        return;
    }
    attron(COLOR_PAIR(COL_BIRD) | A_BOLD);
    mvaddch(y, 10, BIRD_CHAR);
    attroff(COLOR_PAIR(COL_BIRD) | A_BOLD);
}

//draw UI for when the game is being played

static void draw_hud(Game *game){
    attron(COLOR_PAIR(COL_UI) | A_BOLD);
    mvprintw(0,1,"SCORE: %d HS: %d LVL: %d MODE:%s",
                game->score, game->high_score[game->mode][game->diff], game->level, modes[game->mode].name);
    attroff(COLOR_PAIR(COL_UI) | A_BOLD);
    attron(COLOR_PAIR(COL_BACKGROUND));
    mvprintw(HEIGHT - 1, 1, "Grav:%.2fg", modes[game->mode].gravity / 0.20);
    mvprintw(HEIGHT - 1, WIDTH - 30, "SPC=JUMP P=PAUSE  R=RESTART  Q=QUIT");
    attroff(COLOR_PAIR(COL_BACKGROUND));
    if (game->msg_timer > 0 && game->message[0] != '\0'){
        int x = (WIDTH - (int)strlen(game->message)) / 2;
        attron(COLOR_PAIR(COL_TITLE) | A_BOLD | A_BLINK);
        mvprintw(HEIGHT / 2 - 3, x, "%s", game->message);
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD | A_BLINK);
    }
}

static void draw(Game *game){
    erase();
    draw_bg(game);
    draw_border();
    draw_pipes(game);
    draw_bird(game);
    draw_hud(game);
    refresh();
}

//always be ready to for input like Q or P

static void handle_input(Game *game){
    char ch = getch();
    if(ch == ERR)
        return;
    
    if(game->state == PLAY){
        if(ch == ' ' || ch == 'w' || ch == KEY_UP){
            game->bird.speed = modes[game->mode].jump_speed;
            jump();
        }
        else if (ch == 'p')
            game->state = PAUSE;
        else if (ch == 'r')
            init_game(game);
        else if(ch == 'q'){
            update_high_score(game->high_score);
            endwin();
            exit(0);
        }
    }
    else if(game->state == PAUSE){
        if (ch == 'p')
            game->state = PLAY;
        else if (ch == 'r')
            init_game(game);
        else if(ch == 'q'){
            update_high_score(game->high_score);
            endwin();
            exit(0);
        }
    }
}

static void draw_pause(Game *game){
    draw(game);
    int x = WIDTH/2 - 12;
    int y = HEIGHT/2;

    attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
    mvprintw(y, x, "***PAUSE***");
    attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);

    attron(COLOR_PAIR(COL_UI));
    mvprintw(y,     x, "P / SPACE  =  Resume");
    mvprintw(y + 1, x, "R =  Restart");
    mvprintw(y + 2, x, "Q =  Quit");
    attroff(COLOR_PAIR(COL_UI));
    refresh();
}

//draw menu for when the bird dies

static int draw_game_over(Game *game){
    update_high_score(game->high_score);
    int x = WIDTH/2;
    int y = HEIGHT/2;
    const char* diff[3]={"EASY","MEDIUM","HARD"};

    while(1){
        clear();
        attron(COLOR_PAIR(COL_DANGER) | A_BOLD | A_BLINK);
        mvprintw(y-6, x-5, "***GAME OVER***");
        attroff(COLOR_PAIR(COL_DANGER) | A_BOLD | A_BLINK);

        attron(COLOR_PAIR(COL_UI) | A_BOLD);
        mvprintw(y - 4, x - 10, "Your Score: %d", game->score);
        mvprintw(y - 3, x - 10, "Level Reached: %d", game->level);
        mvprintw(y - 2, x - 10, "Mode: %s  (%s)", modes[game->mode].name, modes[game->mode].desc);
        attroff(COLOR_PAIR(COL_UI) | A_BOLD);

        attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
        mvprintw(y, x - 12, "--- HIGH SCORE TABLE ---");
        mvprintw(y + 1, x - 6, "%s  EASY  NORMAL  HARD", "");
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);

        for (int i = 0; i < MAPS; i++){
            attron(COLOR_PAIR(COL_UI));
            mvprintw(y + 2 + i, x - 16, "%-10s  %-6d  %-6d  %-6d", 
                modes[i].name, game->high_score[i][0], game->high_score[i][1],game->high_score[i][2]);
            attroff(COLOR_PAIR(COL_UI));
        }

        attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
        mvprintw(y + 7, x - 19, "[ R ] Restart \t [ M ] Main Menu \t [ Q ] Quit");
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);
 
        refresh();
        usleep(80000);
 
        int ch = getch();
        if (ch == 'r'){ 
            init_game(game); 
            return 0; 
        }
        else if(ch == 'm'){
            return 2;
        }
        else if (ch == 'q'){ 
            endwin(); 
            exit(0); 
        }
    }
}

//draw a menu to select mode

static Mode select_mode(){
    int curr_mode = 0;
    while(1){
        clear();
        int x = WIDTH/2;
        int y = HEIGHT/2;
        attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
        mvprintw(y - 7, x - 8, "CHOOSE YOUR MODE");
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);

        attron(COLOR_PAIR(COL_BACKGROUND));
        mvprintw(y - 5, x - 15, "Each mode has different Gravity");
        mvprintw(y - 3, x - 6, "Choose a mode!");
        attroff(COLOR_PAIR(COL_BACKGROUND));

        for (int i = 0; i < MAPS; i++){
            if (i == curr_mode){
                attron(COLOR_PAIR(COL_DANGER) | A_BOLD | A_REVERSE);
                mvprintw(y - 1 + i * 2, x - 18, " %-8s  %s ", modes[i].name, modes[i].desc);
                attroff(COLOR_PAIR(COL_DANGER) | A_BOLD | A_REVERSE);
            } 
            else{
                attron(COLOR_PAIR(COL_UI));
                mvprintw(y - 1 + i * 2, x - 18, " %-8s  %s", modes[i].name, modes[i].desc);
                attroff(COLOR_PAIR(COL_UI));
            }
        }
 
        attron(COLOR_PAIR(COL_BACKGROUND));
        mvprintw(y + 7, x - 17, "UP/DOWN to choose,  ENTER to confirm");
        attroff(COLOR_PAIR(COL_BACKGROUND));
 
        refresh();
 
        int ch = getch();
        if (ch == KEY_UP) 
            curr_mode = (curr_mode + MAPS - 1) % MAPS;
        else if (ch == KEY_DOWN) 
            curr_mode = (curr_mode + 1) % MAPS;
        else if (ch == '\n' || ch == ' ') 
            break;
        if (ch == 'q'){
            endwin();
            exit(0);
        }
    }
    return (Mode)curr_mode;
}

//draw menu to select diffcult

static Difficulty select_diff(){
    static const char *diffs[3]  ={ "EASY",   "NORMAL",  "HARD"  };
    static const char *descs[3]  ={ "Wide gap","Med gap", "Tight gap" };
    int curr_diff = 1;
 
    while (1){
        clear();
        int x = WIDTH / 2;
        int y = HEIGHT / 2;
 
        attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
        mvprintw(y - 5, x - 9, "SELECT DIFFICULTY");
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);
 
        for (int i = 0; i < 3; i++){
            if (i == curr_diff){
                attron(COLOR_PAIR(COL_DANGER) | A_BOLD | A_REVERSE);
                mvprintw(y - 2 + i * 2, x - 10, " %-8s  %s ", diffs[i], descs[i]);
                attroff(COLOR_PAIR(COL_DANGER) | A_BOLD | A_REVERSE);
            }
            else{
                attron(COLOR_PAIR(COL_UI));
                mvprintw(y - 2 + i * 2, x - 10, "  %-8s  %s", diffs[i], descs[i]);
                attroff(COLOR_PAIR(COL_UI));
            }
        }
 
        attron(COLOR_PAIR(COL_BACKGROUND));
        mvprintw(y + 6, x - 17, "UP/DOWN to choose,  ENTER to confirm");
        attroff(COLOR_PAIR(COL_BACKGROUND));
 
        refresh();
 
        int ch = getch();
        if (ch == KEY_UP) 
            curr_diff = (curr_diff + 2) % 3;
        if (ch == KEY_DOWN) 
            curr_diff = (curr_diff + 1) % 3;
        if (ch == '\n' || ch == ' ') 
            break;
        if (ch == 'q'){
            endwin();
            exit(0);
        }
    }
    return (Difficulty)curr_diff;
}

//draw main menu

static void menu(){
    static const char *title[] ={
        " _|_|_|_|  _|         _|_|     _|_|_|   _|_|_|   _|     _|     _|_|_|    _|_|_|  _|_|_|   _|_|_|   ",
        " _|       _|        _|    _|  _|    _| _|    _|   _|  _|       _|    _|    _|    _|    _| _|    _|",
        " _|_|_|   _|        _|_|_|_|  _|_|_|   _|_|_|       _|         _|_|_|      _|    _|_|_|   _|    _|",
        " _|       _|        _|    _|  _|       _|           _|         _|    _|    _|    _|  _|   _|    _|",
        " _|       _|_|_|_|  _|    _|  _|       _|           _|         _|_|_|    _|_|_|  _|    _| _|_|_|  "
    };
    int frame = 0;
    while (1){
        clear();
        int x = WIDTH / 2;
        int y = 2;

        attron(COLOR_PAIR(COL_TITLE) | A_BOLD);
        for (int i = 0; i < 5; i++){
            int len = (int)strlen(title[i]);
            mvprintw(y + i, 0, "%s", title[i]);
        }
        attroff(COLOR_PAIR(COL_TITLE) | A_BOLD);

        attron(COLOR_PAIR(COL_BACKGROUND));
        mvprintw(y + 13, x - 16, "SPACE  -  Flap");
        mvprintw(y + 14, x - 16, "P / R / Q -  Pause / Restart / Quit");
        attroff(COLOR_PAIR(COL_BACKGROUND));
 
            attron(COLOR_PAIR(COL_DANGER) | A_BOLD | A_BLINK);
            mvprintw(HEIGHT - 3, x - 11, "Press ENTER to start !");
            attroff(COLOR_PAIR(COL_DANGER) | A_BOLD | A_BLINK);
 
        refresh();
        usleep(60000);
 
        int ch = getch();
        if (ch == '\n' || ch == ' ') 
            return;
        if (ch == 'q'){ 
            endwin(); 
            exit(0); 
        }
        frame++;
    }
}

int main(){
    srand((unsigned)time(NULL));//required for rand to work properly
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    idlok(stdscr, FALSE);
    colors();
    int hs[MAPS][3];
    load_high_score(hs);
    Game *game = (Game *)malloc(sizeof(Game));
    memset(game, 0, sizeof(Game));
    memcpy(game->high_score, hs, sizeof(hs));

    while(1){
        menu();
        Mode mode = select_mode();
        Difficulty diff = select_diff();
        game->mode = mode;
        game->diff = diff;
        init_game(game);

        int back_to_menu = 0;
        while(!back_to_menu){
            handle_input(game);
            switch(game->state){
                
                case PLAY:
                    update(game);
                    draw(game);
                    break;
                    
                case PAUSE:
                    draw_pause(game);
                    break;
                
                case FINISH:
                    if(draw_game_over(game) == 2)
                        back_to_menu = 1;
                    break;
            }
            int del = 55000-game->level*1200;
            if(del < 18000)
                del = 18000;
            usleep(del);
        }
    }
    update_high_score(game->high_score);
    endwin();
    return 0;
}
