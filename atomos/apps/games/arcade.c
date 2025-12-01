/*
 * AtomOS Arcade Games
 * Classic arcade game implementations
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* ==================== SNAKE ==================== */
#define SNAKE_SIZE 10
#define SNAKE_MAX_LEN 100

static struct {
    int x[SNAKE_MAX_LEN], y[SNAKE_MAX_LEN];
    int length;
    int dir_x, dir_y;
    int food_x, food_y;
    int speed;
} snake;

static void snake_spawn_food(void) {
    snake.food_x = game_rand_range(1, 38) * SNAKE_SIZE;
    snake.food_y = game_rand_range(1, 28) * SNAKE_SIZE;
}

void snake_init(void) {
    snake.length = 5;
    snake.dir_x = 1;
    snake.dir_y = 0;
    snake.speed = 100;
    for (int i = 0; i < snake.length; i++) {
        snake.x[i] = (10 - i) * SNAKE_SIZE;
        snake.y[i] = 15 * SNAKE_SIZE;
    }
    snake_spawn_food();
}

/* ==================== TETRIS ==================== */
#define TETRIS_W 10
#define TETRIS_H 20
#define TETRIS_BLOCK 15

static struct {
    int board[TETRIS_H][TETRIS_W];
    int piece[4][4];
    int piece_x, piece_y;
    int piece_type;
    int next_piece;
    int lines;
    int drop_speed;
} tetris;

static const int tetris_pieces[7][4][4] = {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},  /* I */
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},  /* O */
    {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},  /* T */
    {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},  /* L */
    {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},  /* J */
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},  /* S */
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},  /* Z */
};

void tetris_init(void) {
    memset(&tetris, 0, sizeof(tetris));
    tetris.piece_x = TETRIS_W / 2 - 2;
    tetris.piece_y = 0;
    tetris.piece_type = game_rand() % 7;
    tetris.next_piece = game_rand() % 7;
    tetris.drop_speed = 500;
    memcpy(tetris.piece, tetris_pieces[tetris.piece_type], sizeof(tetris.piece));
}

/* ==================== PONG ==================== */
static struct {
    int p1_y, p2_y;
    int ball_x, ball_y;
    int ball_dx, ball_dy;
    int p1_score, p2_score;
    int paddle_h;
    int ball_size;
} pong;

void pong_init(void) {
    pong.paddle_h = 60;
    pong.ball_size = 8;
    pong.p1_y = 120;
    pong.p2_y = 120;
    pong.ball_x = 200;
    pong.ball_y = 150;
    pong.ball_dx = 3;
    pong.ball_dy = 2;
    pong.p1_score = 0;
    pong.p2_score = 0;
}

/* ==================== BREAKOUT ==================== */
#define BREAKOUT_COLS 10
#define BREAKOUT_ROWS 5

static struct {
    int bricks[BREAKOUT_ROWS][BREAKOUT_COLS];
    int paddle_x;
    int ball_x, ball_y;
    int ball_dx, ball_dy;
    int bricks_left;
} breakout;

void breakout_init(void) {
    breakout.paddle_x = 160;
    breakout.ball_x = 200;
    breakout.ball_y = 280;
    breakout.ball_dx = 2;
    breakout.ball_dy = -3;
    breakout.bricks_left = BREAKOUT_COLS * BREAKOUT_ROWS;
    for (int r = 0; r < BREAKOUT_ROWS; r++) {
        for (int c = 0; c < BREAKOUT_COLS; c++) {
            breakout.bricks[r][c] = BREAKOUT_ROWS - r;
        }
    }
}

/* ==================== SPACE INVADERS ==================== */
#define INVADERS_COLS 11
#define INVADERS_ROWS 5

static struct {
    int aliens[INVADERS_ROWS][INVADERS_COLS];
    int alien_x, alien_y;
    int alien_dir;
    int player_x;
    int bullet_x, bullet_y;
    bool bullet_active;
    int aliens_left;
} invaders;

void space_invaders_init(void) {
    invaders.alien_x = 20;
    invaders.alien_y = 30;
    invaders.alien_dir = 1;
    invaders.player_x = 180;
    invaders.bullet_active = false;
    invaders.aliens_left = INVADERS_COLS * INVADERS_ROWS;
    for (int r = 0; r < INVADERS_ROWS; r++) {
        for (int c = 0; c < INVADERS_COLS; c++) {
            invaders.aliens[r][c] = 1;
        }
    }
}

/* ==================== ASTEROIDS ==================== */
#define MAX_ASTEROIDS 20
#define MAX_BULLETS 5

static struct {
    int ship_x, ship_y;
    int ship_angle;
    int ship_dx, ship_dy;
    struct { int x, y, dx, dy, size; bool active; } asteroids[MAX_ASTEROIDS];
    struct { int x, y, dx, dy; bool active; } bullets[MAX_BULLETS];
    int asteroid_count;
} asteroids_game;

void asteroids_init(void) {
    asteroids_game.ship_x = 200;
    asteroids_game.ship_y = 200;
    asteroids_game.ship_angle = 0;
    asteroids_game.ship_dx = 0;
    asteroids_game.ship_dy = 0;
    asteroids_game.asteroid_count = 4;
    
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids_game.asteroids[i].active = (i < 4);
        if (i < 4) {
            asteroids_game.asteroids[i].x = game_rand() % 400;
            asteroids_game.asteroids[i].y = game_rand() % 400;
            asteroids_game.asteroids[i].dx = game_rand_range(-2, 2);
            asteroids_game.asteroids[i].dy = game_rand_range(-2, 2);
            asteroids_game.asteroids[i].size = 30;
        }
    }
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        asteroids_game.bullets[i].active = false;
    }
}

/* ==================== FROGGER ==================== */
#define FROGGER_LANES 5

static struct {
    int frog_x, frog_y;
    struct { int x, speed, width; } cars[FROGGER_LANES][4];
    struct { int x, speed, width; } logs[FROGGER_LANES][3];
    int frogs_home;
    bool homes[5];
} frogger;

void frogger_init(void) {
    frogger.frog_x = 180;
    frogger.frog_y = 380;
    frogger.frogs_home = 0;
    
    for (int i = 0; i < 5; i++) {
        frogger.homes[i] = false;
    }
    
    for (int lane = 0; lane < FROGGER_LANES; lane++) {
        for (int i = 0; i < 4; i++) {
            frogger.cars[lane][i].x = i * 100;
            frogger.cars[lane][i].speed = (lane % 2 == 0) ? 2 : -2;
            frogger.cars[lane][i].width = 40;
        }
    }
}

/* ==================== PAC-MAN ==================== */
#define PACMAN_W 28
#define PACMAN_H 31
#define PACMAN_GHOSTS 4

static struct {
    int pac_x, pac_y;
    int pac_dir;
    int pac_next_dir;
    struct { int x, y, dir; int mode; } ghosts[PACMAN_GHOSTS];
    int dots[PACMAN_H][PACMAN_W];
    int dots_left;
    bool power_mode;
    int power_timer;
} pacman;

void pacman_init(void) {
    pacman.pac_x = 14 * 8;
    pacman.pac_y = 23 * 8;
    pacman.pac_dir = 0;
    pacman.dots_left = 0;
    pacman.power_mode = false;
    
    for (int y = 0; y < PACMAN_H; y++) {
        for (int x = 0; x < PACMAN_W; x++) {
            pacman.dots[y][x] = 1;
            pacman.dots_left++;
        }
    }
    
    /* Ghost positions */
    int ghost_starts[4][2] = {{12, 14}, {13, 14}, {14, 14}, {15, 14}};
    for (int i = 0; i < PACMAN_GHOSTS; i++) {
        pacman.ghosts[i].x = ghost_starts[i][0] * 8;
        pacman.ghosts[i].y = ghost_starts[i][1] * 8;
        pacman.ghosts[i].dir = game_rand() % 4;
        pacman.ghosts[i].mode = 0;
    }
}

/* ==================== FLAPPY BIRD ==================== */
#define FLAPPY_PIPES 4

static struct {
    int bird_y;
    int bird_vel;
    struct { int x, gap_y; } pipes[FLAPPY_PIPES];
    int scroll;
} flappy;

void flappy_init(void) {
    flappy.bird_y = 200;
    flappy.bird_vel = 0;
    flappy.scroll = 0;
    
    for (int i = 0; i < FLAPPY_PIPES; i++) {
        flappy.pipes[i].x = 300 + i * 150;
        flappy.pipes[i].gap_y = game_rand_range(100, 300);
    }
}

/* ==================== RUNNER ==================== */
#define RUNNER_OBSTACLES 5

static struct {
    int player_y;
    int vel_y;
    bool jumping;
    bool ducking;
    struct { int x, type; } obstacles[RUNNER_OBSTACLES];
    int speed;
    int distance;
} runner;

void runner_init(void) {
    runner.player_y = 250;
    runner.vel_y = 0;
    runner.jumping = false;
    runner.ducking = false;
    runner.speed = 5;
    runner.distance = 0;
    
    for (int i = 0; i < RUNNER_OBSTACLES; i++) {
        runner.obstacles[i].x = 500 + i * 200;
        runner.obstacles[i].type = game_rand() % 3;
    }
}

/* ==================== SHOOTER ==================== */
#define SHOOTER_ENEMIES 10
#define SHOOTER_BULLETS 20

static struct {
    int player_x, player_y;
    struct { int x, y, type; bool active; } enemies[SHOOTER_ENEMIES];
    struct { int x, y, dy; bool active; bool player; } bullets[SHOOTER_BULLETS];
    int spawn_timer;
} shooter;

void shooter_init(void) {
    shooter.player_x = 200;
    shooter.player_y = 350;
    shooter.spawn_timer = 0;
    
    for (int i = 0; i < SHOOTER_ENEMIES; i++) {
        shooter.enemies[i].active = false;
    }
    for (int i = 0; i < SHOOTER_BULLETS; i++) {
        shooter.bullets[i].active = false;
    }
}

/* ==================== PLATFORMER ==================== */
#define PLAT_TILES_W 50
#define PLAT_TILES_H 15

static struct {
    int player_x, player_y;
    int vel_x, vel_y;
    bool on_ground;
    int tiles[PLAT_TILES_H][PLAT_TILES_W];
    int coins_collected;
    int scroll_x;
} platformer;

void platformer_init(void) {
    platformer.player_x = 50;
    platformer.player_y = 200;
    platformer.vel_x = 0;
    platformer.vel_y = 0;
    platformer.on_ground = false;
    platformer.coins_collected = 0;
    platformer.scroll_x = 0;
    
    /* Generate level */
    memset(platformer.tiles, 0, sizeof(platformer.tiles));
    for (int x = 0; x < PLAT_TILES_W; x++) {
        platformer.tiles[14][x] = 1;  /* Ground */
        if (x % 5 == 0 && x > 5) {
            platformer.tiles[10][x] = 1;  /* Platforms */
        }
    }
}

/* ==================== RACING ==================== */
#define RACING_CARS 5

static struct {
    int player_x;
    int player_speed;
    struct { int x, y, speed; } cars[RACING_CARS];
    int road_offset;
    int distance;
} racing;

void racing_init(void) {
    racing.player_x = 200;
    racing.player_speed = 5;
    racing.road_offset = 0;
    racing.distance = 0;
    
    for (int i = 0; i < RACING_CARS; i++) {
        racing.cars[i].x = game_rand_range(100, 300);
        racing.cars[i].y = -100 - i * 150;
        racing.cars[i].speed = game_rand_range(2, 4);
    }
}
