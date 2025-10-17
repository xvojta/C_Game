#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

//#define _WIN
#ifdef _WIN
#include <conio.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#define mvprintw(y, x, fmt, ...) printf(fmt, ##__VA_ARGS__)
#define addch(ch) putchar(ch)

void clear_screen() {
    printf("\x1b[2J\x1b[H");
}

void graphic_init() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}


int get_input(char *input) {
    if(kbhit()) {
        *input = getch();
        return 1;
    }
    return 0;
}    

void  graphic_exit() {
    clear_screen();
}

#else
#include <unistd.h>
#include <ncurses.h>

void clear_screen() {
    clear();
}

void graphic_init() {
    initscr();            // start ncurses mode
    noecho();             // don't echo pressed keys
    cbreak();             // disable line buffering
    nodelay(stdscr, TRUE); // make getch() non-blocking
    keypad(stdscr, TRUE); // enable arrow keys
}

int get_input(char *input) {
    char key_pressed = getch();
    if(key_pressed != ERR) {
        *input = key_pressed;
        return 1;
    }
    return 0;
}

void  graphic_exit() {
    endwin();
}

#endif

#define WIDTH 100
#define HEIGHT 20
#define HGH_DIST 2.0
#define LVL_PREFIX 100
#define NUM_LVLS 2

typedef struct {
    int x;
    int y;
} Vector;

/**
 * @brief Square of a number
 */
double sq(double a) {
    return pow(a,2);
}

/**
 * @brief Distance between two points
 * 
 * @param a First point
 * @param b Second point
 * @return double Distance
 */
double points_dist(Vector a, Vector b) {
    return sqrt(sq(a.x - b.x) + sq( HGH_DIST * (a.y - b.y) ));     
}

/**
 * @brief Distance from point to line segment
 * 
 * @param ln_1 Line segment start
 * @param ln_2 Line segment end
 * @param point Point to measure from
 * @return double Distance
 */
double point_from_sg_dist(Vector ln_1, Vector ln_2, Vector point) {
    double pt_ln1 = points_dist(point, ln_1);
    double pt_ln2 = points_dist(point, ln_2);
    double ln1_ln2 = points_dist(ln_1, ln_2);

    if(sq(ln1_ln2) + sq(pt_ln1) >= sq(pt_ln2) && sq(ln1_ln2) + sq(pt_ln2) >= sq(pt_ln1)) {
        //convex
        double a = point.x * (ln_2.y - ln_1.y);
        double b = point.y * (ln_1.x - ln_2.x);
        double c = ln_1.y * ln_2.x;
        double d = ln_1.x * ln_2.y;
        double e = sq(ln_2.y - ln_1.y);
        double f = sq(ln_1.x - ln_2.x);
        
        return (abs(a + b + c - d) /
                sqrt(e + f));
    } else {
        //concave
        return fmin(pt_ln1, pt_ln2);
    }
}

/**
 * @brief Distance from point to circle
 * 
 * @param circ_mid Circle center
 * @param radius Circle radius
 * @param point Point to measure from
 * @param full If true, measures distance to outer edge of circle, if false, measures distance to nearest edge (inside or outside)
 * @return double Distance
 */
double point_from_circle(Vector circ_mid, double radius, Vector point, bool full) {
    if(full) {
        return fmax( points_dist(circ_mid, point) - radius, 0 );
    }

    return abs( points_dist(circ_mid, point) - radius );
}

/**
 * @brief Insert a line into the environment
 * 
 * @param enviroment2d 2D array representing the environment
 * @param start Line start
 * @param end Line end
 * @param thickness Line thickness
 * @param filler Value to fill the line with
 */
void insert_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness, int filler) {
    int x_min = fmax(fmin(start.x, end.x) - thickness, 0);
    int x_max = fmin(fmax(start.x, end.x) + thickness, WIDTH - 1);
    int y_min = fmax(fmin(start.y, end.y) - thickness, 0);
    int y_max = fmin(fmax(start.y, end.y) + thickness, HEIGHT - 1);

    for (int x = x_min; x <= x_max; x++)
    {
        for (int y = y_min; y <= y_max; y++)
        {
            if(point_from_sg_dist(start, end, (Vector){x, y}) < thickness)
                enviroment2d[x][y] = filler;
        }
    }
}

/**
 * @brief Draw a line in the environment
 * 
 * @param enviroment2d 2D array representing the environment
 * @param start Line start
 * @param end Line end
 * @param thickness Line thickness
 */
void draw_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness) {
    insert_line(enviroment2d, start, end, thickness, 5);
}

/**
 * @brief Draw a portal line in the environment
 * 
 * @param enviroment2d 2D array representing the environment
 * @param start Line start
 * @param end Line end
 * @param thickness Line thickness
 * @param level Level to which the portal leads
 */
void draw_portal_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness, int level) {
    insert_line(enviroment2d, start, end, thickness, LVL_PREFIX + level);
}

/**
 * @brief Set a line area in the environment to empty
 * 
 * @param enviroment2d 2D array representing the environment
 * @param start Line start
 * @param end Line end
 * @param thickness Line thickness
 */
void remove_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness) {
    insert_line(enviroment2d, start, end, thickness, 0);
}

/**
 * @brief Insert a circle into the environment
 * 
 * @param enviroment2d 2D array representing the environment
 * @param mid Circle center
 * @param radius Circle radius
 * @param thickness Circle thickness
 * @param full If true, draws a full circle, if false, draws only the edge
 * @param filler Value to fill the circle with
 */
void insert_circle(int enviroment2d[WIDTH][HEIGHT], Vector mid, double radius, double thickness, bool full, int filler) {
    int x_min = fmax(mid.x - radius - thickness, 0);
    int x_max = fmin(mid.x + radius + thickness, WIDTH - 1);
    int y_min = fmax(mid.y - radius - thickness, 0);
    int y_max = fmin(mid.y + radius + thickness, HEIGHT - 1);

    for (int x = x_min; x <= x_max; x++)
    {
        for (int y = y_min; y <= y_max; y++)
        {
            if(point_from_circle(mid, radius, (Vector){x, y}, full) < thickness)
                enviroment2d[x][y] = filler;
        }
    }
}

/**
 * @brief Draw a circle in the environment
 * 
 * @param enviroment2d 2D array representing the environment
 * @param mid Circle center
 * @param radius Circle radius
 * @param thickness Circle thickness
 * @param full If true, draws a full circle, if false, draws only the edge
 */
void draw_circle(int enviroment2d[WIDTH][HEIGHT], Vector mid, double radius, double thickness, bool full) {
    insert_circle(enviroment2d, mid, radius, thickness, full, 5);
}

/**
 * @brief Set a circle area in the environment to empty
 * 
 * @param enviroment2d 2D array representing the environment
 * @param mid Circle center
 * @param radius Circle radius
 * @param thickness Circle thickness
 * @param full If true, removes a full circle, if false, removes only the edge
 */
void remove_circle(int enviroment2d[WIDTH][HEIGHT], Vector mid, double radius, double thickness, bool full) {
    insert_circle(enviroment2d, mid, radius, thickness, full, 0);
}

/**
 * @brief Spawn an item at a random empty position in the environment
 * 
 * @param enviroment 2D array representing the environment
 * @param max_tries Maximum number of attempts to find an empty position
 * @param filler Value to fill the spawned item with
 */
void random_spawn(int enviroment[WIDTH][HEIGHT], int max_tries, int filler) {
    for(int i = 0; i < max_tries; i++) {
        int rand_x = rand() % 101;
        int rand_y = rand() % 21;
        if(enviroment[rand_x][rand_y] == 0) {
            enviroment[rand_x][rand_y] = filler;
            return;
        }
    }
}

/**
 * @brief Spawn a coin at a random empty position in the environment
 * 
 * @param enviroment 2D array representing the environment
 */
void spawn_coin(int enviroment[WIDTH][HEIGHT]) {
    random_spawn(enviroment, 200, 2);
}

/**
 * @brief Spawn a chest at a random empty position in the environment
 * 
 * @param enviroment 2D array representing the environment
 */
void spawn_chest(int enviroment[WIDTH][HEIGHT]) {
    random_spawn(enviroment, 200, 3);
}

/**
 * @brief Spawn a key at a random empty position in the environment
 * 
 * @param enviroment 2D array representing the environment
 */
void spawn_key(int enviroment[WIDTH][HEIGHT]) {
    random_spawn(enviroment, 200, 4);
}

/**
 * @brief Move the player to a new position if valid
 * 
 * @param enviroment 2D array representing the environment
 * @param player_pos Current player position
 * @param new_pos Desired new player position
 * @param score Pointer to the player's score
 * @param new_level Pointer to the new level (if changed)
 * @return int 1 if level changed, 0 otherwise
 */
int playerMoved(int enviroment[WIDTH][HEIGHT], Vector *player_pos, Vector *new_pos, int *score, int *new_level) {
    int o_x = player_pos->x;
    int o_y = player_pos->y;
    int n_x = new_pos->x;
    int n_y = new_pos->y;
    if(n_x >= 0 && n_x < WIDTH && n_y >= 0 && n_y < HEIGHT)
    {
        int new_pos_env = enviroment[n_x][n_y];
        if(new_pos_env == 0 || new_pos_env == 2) {
            if (new_pos_env == 2) {
                *score = *score + 1;
                spawn_coin(enviroment);
            }
            *player_pos = *new_pos;
            enviroment[o_x][o_y] = 0;
            enviroment[n_x][n_y] = 1;
        } else if (new_pos_env >= LVL_PREFIX) {
            *new_level = new_pos_env - LVL_PREFIX;
            return 1;
        }
        
    }
    return 0;
}

/**
 * @brief Change the current level if needed
 * 
 * @param enviroment 3D array representing the environment with multiple levels
 * @param new_level Pointer to the new level
 * @param current_level Pointer to the current level
 * @param player_pos Pointer to the player's position
 * @param spawn_points Array of spawn points for each level
 */
void change_level(int enviroment[NUM_LVLS][WIDTH][HEIGHT], int *new_level, int *current_level, Vector *player_pos, Vector spawn_points[NUM_LVLS]) {
    if(*new_level != *current_level) {
        spawn_points[*current_level] = *player_pos;
        enviroment[*current_level][player_pos->x][player_pos->y] = 0;

        *current_level = *new_level;
        *player_pos = spawn_points[*current_level];
        enviroment[*current_level][player_pos->x][player_pos->y] = 1;
    }
}

/**
 * @brief Render the environment and score to the screen
 * 
 * @param enviroment2d 2D array representing the environment
 * @param score Player's score
 */
void render(int enviroment2d[WIDTH][HEIGHT], int score) {
    clear_screen();
    mvprintw(0, 0, "Score: %d\n", score);
    for(int y = 0; y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            switch (enviroment2d[x][y])
            {
            case 0:
                addch(' ');
                break;

            case 1:
                addch('x');
                break;

            case 2:
                addch('o'); // coin
                break;

            case 3:
                addch('M'); //chest
                break;

            case 4:
                addch('&'); //key
                break;

            case 5:
                addch('#'); //wall
                break;

            default:
                addch(' ');
                break;
            }
        }
        addch('\n');
    }
}

/**
 * @brief Copy one environment to another
 * 
 * @param cpy_to Destination 2D array
 * @param cpy_from Source 2D array
 */
void cpy_enviroment(int cpy_to[WIDTH][HEIGHT], int cpy_from[WIDTH][HEIGHT]) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            cpy_to[x][y] = cpy_from[x][y];
        }
    }
}

/**
 * @brief Initialize the environment with predefined levels and spawn points
 * 
 * @param enviroment 3D array representing the environment with multiple levels
 * @param spawn_points Array of spawn points for each level
 */
void initialize_enviroment(int enviroment[NUM_LVLS][WIDTH][HEIGHT], Vector spawn_points[NUM_LVLS]) {

    //level 0
    draw_line(enviroment[0], (Vector){0, 0}, (Vector){WIDTH - 1, 0}, 1.0);
    draw_line(enviroment[0], (Vector){WIDTH - 1, 0}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment[0], (Vector){0, HEIGHT - 1}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment[0], (Vector){0, 0}, (Vector){0, HEIGHT - 1}, 1.0);
    draw_portal_line(enviroment[0], (Vector){WIDTH - 1, 8}, (Vector){WIDTH - 1, 12}, 1.0, 1);

    draw_line(enviroment[0], (Vector){70,0}, (Vector){70,8}, 1.0);
    draw_line(enviroment[0], (Vector){70,12}, (Vector){70,HEIGHT - 1}, 1.0);
    draw_circle(enviroment[0], (Vector) {35, 10}, 8.0, 1.0, false);
    remove_line(enviroment[0], (Vector){27, 10}, (Vector){43, 10}, 2.0);
    spawn_coin(enviroment[0]);
    spawn_chest(enviroment[0]);

    spawn_points[0] = (Vector){50, 10};

    //level 1
    draw_line(enviroment[1], (Vector){0, 0}, (Vector){WIDTH - 1, 0}, 1.0);
    draw_line(enviroment[1], (Vector){WIDTH - 1, 0}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment[1], (Vector){0, HEIGHT - 1}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment[1], (Vector){0, 0}, (Vector){0, HEIGHT - 1}, 1.0);
    draw_portal_line(enviroment[1], (Vector){0, 8}, (Vector){0, 12}, 1.0, 0);

    draw_line(enviroment[1], (Vector){70,0}, (Vector){70,8}, 1.0);
    draw_line(enviroment[1], (Vector){70,12}, (Vector){70,HEIGHT - 1}, 1.0);
    draw_circle(enviroment[1], (Vector) {20, 10}, 10.0, 1.0, false);
    remove_line(enviroment[1], (Vector){27, 10}, (Vector){43, 10}, 2.0);
    spawn_coin(enviroment[1]);
    spawn_key(enviroment[1]);

    spawn_points[1] = (Vector){1, 9};
}

int main() {
    Vector player_pos = {50, 10};
    int score = 0;
    int level = 0;
    int keys_collected = 0;
    Vector spawn_points[NUM_LVLS];
    srand(time(NULL));  

    graphic_init();

    int enviroment[NUM_LVLS][WIDTH][HEIGHT] = {0};

    initialize_enviroment(enviroment, spawn_points);

    mvprintw(0, 0, "Press any key to start...");

    do {
        char input;
        if(get_input(&input)) {
            Vector new_pos = player_pos;
            switch (input)
            {
                case 'a':
                    new_pos.x--;
                    break;
                case 'd':
                    new_pos.x++;
                    break;
                case 'w':
                    new_pos.y--;
                    break;
                case 's':
                    new_pos.y++;
                    break;
                case 'x':
                    graphic_exit();
                    return 0;
                default:
                    break;
            }
            //int rend_env[WIDTH][HEIGHT];
            //cpy_enviroment(rend_env, enviroment);

            int new_level;
            int event = playerMoved(enviroment[level], &player_pos, &new_pos, &score, &new_level);
            if(event == 1) {
                change_level(enviroment, &new_level, &level, &player_pos, spawn_points);
            }
            render(enviroment[level], score);
	    //usleep(10000);
        }
    } while(1);
    return 0;
}

