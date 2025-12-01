/*
 * AtomOS Sports Games
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* ==================== GOLF ==================== */
#define GOLF_HOLES 9

static struct {
    int ball_x, ball_y;
    int hole_x, hole_y;
    int power;
    int angle;
    int vel_x, vel_y;
    bool in_motion;
    int strokes;
    int current_hole;
    int total_strokes;
    int par[GOLF_HOLES];
    bool obstacles[200][150];
    bool water[200][150];
    bool sand[200][150];
} golf;

void golf_init(void) {
    memset(&golf, 0, sizeof(golf));
    
    golf.ball_x = 50;
    golf.ball_y = 200;
    golf.hole_x = 450;
    golf.hole_y = 200;
    golf.power = 50;
    golf.angle = 0;
    golf.current_hole = 1;
    
    for (int i = 0; i < GOLF_HOLES; i++) {
        golf.par[i] = 3 + game_rand() % 3;
    }
    
    /* Add some obstacles */
    for (int i = 0; i < 5; i++) {
        int ox = game_rand_range(100, 400);
        int oy = game_rand_range(50, 350);
        for (int dy = 0; dy < 30; dy++) {
            for (int dx = 0; dx < 20; dx++) {
                if (oy + dy < 150 && ox + dx < 200) {
                    golf.obstacles[ox + dx][oy + dy] = true;
                }
            }
        }
    }
}

/* ==================== BOWLING ==================== */
#define BOWLING_FRAMES 10

static struct {
    int pins[10];  /* 0=standing, 1=down */
    int ball_x, ball_y;
    int ball_vel_x;
    int ball_spin;
    bool rolling;
    int frame;
    int roll;
    int scores[BOWLING_FRAMES][3];
    int frame_scores[BOWLING_FRAMES];
    int aiming;
    int power;
    int spin;
} bowling;

static void bowling_reset_pins(void) {
    for (int i = 0; i < 10; i++) {
        bowling.pins[i] = 0;
    }
}

void bowling_init(void) {
    memset(&bowling, 0, sizeof(bowling));
    bowling.ball_x = 200;
    bowling.ball_y = 450;
    bowling.frame = 1;
    bowling.roll = 1;
    bowling.power = 50;
    bowling_reset_pins();
}

/* ==================== DARTS ==================== */
static struct {
    int aim_x, aim_y;
    int wobble_x, wobble_y;
    int darts_thrown;
    int current_score;
    int target_score;
    int multiplier;
    int throws_left;
    bool throwing;
    int throw_power;
} darts;

void darts_init(void) {
    memset(&darts, 0, sizeof(darts));
    darts.aim_x = 200;
    darts.aim_y = 200;
    darts.target_score = 501;
    darts.current_score = 501;
    darts.throws_left = 3;
    darts.multiplier = 1;
}

/* ==================== PINBALL ==================== */
#define PINBALL_BUMPERS 5
#define PINBALL_TARGETS 3

static struct {
    int ball_x, ball_y;
    int ball_vx, ball_vy;
    int left_flipper;   /* Angle */
    int right_flipper;
    struct { int x, y, radius; } bumpers[PINBALL_BUMPERS];
    struct { int x, y; bool hit; } targets[PINBALL_TARGETS];
    int balls_left;
    bool ball_in_play;
    int bonus_multiplier;
} pinball;

void pinball_init(void) {
    memset(&pinball, 0, sizeof(pinball));
    
    pinball.ball_x = 380;
    pinball.ball_y = 550;
    pinball.balls_left = 3;
    pinball.bonus_multiplier = 1;
    
    /* Set up bumpers */
    pinball.bumpers[0].x = 200; pinball.bumpers[0].y = 150; pinball.bumpers[0].radius = 25;
    pinball.bumpers[1].x = 150; pinball.bumpers[1].y = 200; pinball.bumpers[1].radius = 20;
    pinball.bumpers[2].x = 250; pinball.bumpers[2].y = 200; pinball.bumpers[2].radius = 20;
    pinball.bumpers[3].x = 175; pinball.bumpers[3].y = 280; pinball.bumpers[3].radius = 15;
    pinball.bumpers[4].x = 225; pinball.bumpers[4].y = 280; pinball.bumpers[4].radius = 15;
    
    /* Set up targets */
    pinball.targets[0].x = 100; pinball.targets[0].y = 100; pinball.targets[0].hit = false;
    pinball.targets[1].x = 200; pinball.targets[1].y = 80; pinball.targets[1].hit = false;
    pinball.targets[2].x = 300; pinball.targets[2].y = 100; pinball.targets[2].hit = false;
}

/* ==================== AIR HOCKEY ==================== */
static struct {
    int puck_x, puck_y;
    int puck_vx, puck_vy;
    int player_x, player_y;
    int cpu_x, cpu_y;
    int player_score, cpu_score;
    int paddle_radius;
    int puck_radius;
    bool goal_scored;
} airhockey;

void airhockey_init(void) {
    memset(&airhockey, 0, sizeof(airhockey));
    
    airhockey.puck_x = 200;
    airhockey.puck_y = 250;
    airhockey.player_x = 200;
    airhockey.player_y = 400;
    airhockey.cpu_x = 200;
    airhockey.cpu_y = 100;
    airhockey.paddle_radius = 30;
    airhockey.puck_radius = 15;
}

/* ==================== POOL ==================== */
#define POOL_BALLS 16

static struct {
    struct {
        int x, y;
        int vx, vy;
        int type;  /* 0=cue, 1-7=solid, 8=8ball, 9-15=stripe */
        bool pocketed;
    } balls[POOL_BALLS];
    int cue_angle;
    int cue_power;
    bool aiming;
    bool balls_moving;
    int player_type;  /* 0=undecided, 1=solid, 2=stripe */
    int player_turn;
    int player_score[2];
} pool;

void pool_init(void) {
    memset(&pool, 0, sizeof(pool));
    
    /* Cue ball */
    pool.balls[0].x = 150;
    pool.balls[0].y = 175;
    pool.balls[0].type = 0;
    
    /* Rack the balls */
    int rack_x = 350;
    int rack_y = 175;
    int ball_r = 10;
    int idx = 1;
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col <= row; col++) {
            if (idx >= POOL_BALLS) break;
            pool.balls[idx].x = rack_x + row * (ball_r * 2);
            pool.balls[idx].y = rack_y + (col - row / 2.0f) * (ball_r * 2);
            pool.balls[idx].type = idx;
            idx++;
        }
    }
    
    pool.aiming = true;
}
