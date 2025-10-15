#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 100
#define HEIGHT 20
#define HGH_DIST 2.0

typedef struct {
    int x;
    int y;
} Vector;

double sq(double a) {
    return pow(a,2);
}

double points_dist(Vector a, Vector b) {
    return sqrt(sq(a.x - b.x) + sq( HGH_DIST * (a.y - b.y) ));     
}

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

double point_from_circle(Vector circ_mid, double radius, Vector point, bool full) {
    if(full) {
        return fmax( points_dist(circ_mid, point) - radius, 0 );
    }

    return abs( points_dist(circ_mid, point) - radius );
}

void clear_screen() {
    clear();
}

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

void draw_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness) {
    insert_line(enviroment2d, start, end, thickness, 5);
}

void remove_line(int enviroment2d[WIDTH][HEIGHT], Vector start, Vector end, double thickness) {
    insert_line(enviroment2d, start, end, thickness, 0);
}

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

void draw_circle(int enviroment2d[WIDTH][HEIGHT], Vector mid, double radius, double thickness, bool full) {
    insert_circle(enviroment2d, mid, radius, thickness, full, 5);
}

void remove_circle(int enviroment2d[WIDTH][HEIGHT], Vector mid, double radius, double thickness, bool full) {
    insert_circle(enviroment2d, mid, radius, thickness, full, 0);
}

void generate_point(int enviroment[WIDTH][HEIGHT], int max_tries) {
    for(int i = 0; /*i < max_tries*/1; i++) {
        int rand_x = rand() % 101;
        int rand_y = rand() % 21;
        if(enviroment[rand_x][rand_y] == 0) {
            enviroment[rand_x][rand_y] = 2;
            return;
        }
    }
}

void playerMoved(int enviroment[WIDTH][HEIGHT], Vector *player_pos, Vector *new_pos, int *score) {
    int o_x = player_pos->x;
    int o_y = player_pos->y;
    int n_x = new_pos->x;
    int n_y = new_pos->y;
    if(n_x >= 0 && n_x < WIDTH && n_y >= 0 && n_y < HEIGHT)
    {
        if(enviroment[n_x][n_y] == 0 || enviroment[n_x][n_y] == 2) {
            if (enviroment[n_x][n_y] == 2) {
                *score = *score + 1;
                generate_point(enviroment, HEIGHT);
            }
            *player_pos = *new_pos;
            enviroment[o_x][o_y] = 0;
            enviroment[n_x][n_y] = 1;
        }
    }
}


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
                addch('o');
                break;

            case 5:
                addch('#'); //wall
                break;

            default:
                break;
            }
        }
        addch('\n');
    }
}

void cpy_enviroment(int cpy_to[WIDTH][HEIGHT], int cpy_from[WIDTH][HEIGHT]) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            cpy_to[x][y] = cpy_from[x][y];
        }
    }
}

int main() {
    Vector player_pos = {50, 10};
    int score = 0;
    srand(time(NULL));  

    initscr();            // start ncurses mode
    noecho();             // don't echo pressed keys
    cbreak();             // disable line buffering
    nodelay(stdscr, TRUE); // make getch() non-blocking
    keypad(stdscr, TRUE); // enable arrow keys

    int enviroment[WIDTH][HEIGHT] = {0};

    draw_line(enviroment, (Vector){0, 0}, (Vector){WIDTH - 1, 0}, 1.0);
    draw_line(enviroment, (Vector){WIDTH - 1, 0}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment, (Vector){0, HEIGHT - 1}, (Vector){WIDTH - 1, HEIGHT - 1}, 1.0);
    draw_line(enviroment, (Vector){0, 0}, (Vector){0, HEIGHT - 1}, 1.0);

    draw_line(enviroment, (Vector){70,0}, (Vector){70,8}, 1.0);
    draw_line(enviroment, (Vector){70,12}, (Vector){70,HEIGHT - 1}, 1.0);
    draw_circle(enviroment, (Vector) {35, 10}, 8.0, 1.0, false);
    remove_line(enviroment, (Vector){27, 10}, (Vector){43, 10}, 2.0);
    generate_point(enviroment, HEIGHT);

    printf("Press any key to start...");

    do {
	char input = getch();
        if(input != ERR) {
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
                    endwin();
                    return 0;
                default:
                    break;
            }
            //int rend_env[WIDTH][HEIGHT];
            //cpy_enviroment(rend_env, enviroment);

            
            playerMoved(enviroment, &player_pos, &new_pos, &score);
            render(enviroment, score);
	    usleep(10000);
        }
    } while(1);
    return 0;
}

