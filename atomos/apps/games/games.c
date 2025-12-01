/*
 * AtomOS Game Framework Implementation
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Global game context */
game_ctx_t *g_game = NULL;

/* Random state */
static uint32_t rand_state = 12345;

/* Game list - all 50 games */
game_info_t game_list[] = {
    {"Snake", "Classic snake game", "AtomOS", 400, 300, snake_init, NULL, NULL, NULL, NULL},
    {"Tetris", "Block stacking puzzle", "AtomOS", 300, 400, tetris_init, NULL, NULL, NULL, NULL},
    {"Pong", "Classic paddle game", "AtomOS", 400, 300, pong_init, NULL, NULL, NULL, NULL},
    {"Breakout", "Brick breaking game", "AtomOS", 400, 400, breakout_init, NULL, NULL, NULL, NULL},
    {"Space Invaders", "Alien shooter", "AtomOS", 400, 400, space_invaders_init, NULL, NULL, NULL, NULL},
    {"Asteroids", "Space shooter", "AtomOS", 400, 400, asteroids_init, NULL, NULL, NULL, NULL},
    {"Frogger", "Road crossing", "AtomOS", 400, 400, frogger_init, NULL, NULL, NULL, NULL},
    {"Pac-Man", "Maze chase", "AtomOS", 400, 400, pacman_init, NULL, NULL, NULL, NULL},
    {"Minesweeper", "Mine finding puzzle", "AtomOS", 300, 300, minesweeper_init, NULL, NULL, NULL, NULL},
    {"Sudoku", "Number puzzle", "AtomOS", 400, 400, sudoku_init, NULL, NULL, NULL, NULL},
    {"2048", "Tile sliding", "AtomOS", 300, 300, game2048_init, NULL, NULL, NULL, NULL},
    {"Memory", "Card matching", "AtomOS", 400, 300, memory_init, NULL, NULL, NULL, NULL},
    {"Puzzle", "Jigsaw puzzle", "AtomOS", 400, 400, puzzle_init, NULL, NULL, NULL, NULL},
    {"Match 3", "Gem matching", "AtomOS", 400, 400, match3_init, NULL, NULL, NULL, NULL},
    {"Sokoban", "Box pushing puzzle", "AtomOS", 400, 400, sokoban_init, NULL, NULL, NULL, NULL},
    {"15 Puzzle", "Sliding tiles", "AtomOS", 300, 300, fifteen_init, NULL, NULL, NULL, NULL},
    {"Solitaire", "Card game", "AtomOS", 600, 400, solitaire_init, NULL, NULL, NULL, NULL},
    {"Blackjack", "21 card game", "AtomOS", 500, 400, blackjack_init, NULL, NULL, NULL, NULL},
    {"Poker", "Texas Hold'em", "AtomOS", 600, 400, poker_init, NULL, NULL, NULL, NULL},
    {"Hearts", "Card game", "AtomOS", 600, 400, hearts_init, NULL, NULL, NULL, NULL},
    {"FreeCell", "Card game", "AtomOS", 600, 400, freecell_init, NULL, NULL, NULL, NULL},
    {"Chess", "Strategy board game", "AtomOS", 400, 400, chess_init, NULL, NULL, NULL, NULL},
    {"Checkers", "Board game", "AtomOS", 400, 400, checkers_init, NULL, NULL, NULL, NULL},
    {"Tic-Tac-Toe", "X and O", "AtomOS", 300, 300, tictactoe_init, NULL, NULL, NULL, NULL},
    {"Connect 4", "Drop discs", "AtomOS", 400, 350, connect4_init, NULL, NULL, NULL, NULL},
    {"Reversi", "Othello", "AtomOS", 400, 400, reversi_init, NULL, NULL, NULL, NULL},
    {"Gomoku", "Five in a row", "AtomOS", 400, 400, gomoku_init, NULL, NULL, NULL, NULL},
    {"Flappy Bird", "Flying game", "AtomOS", 300, 400, flappy_init, NULL, NULL, NULL, NULL},
    {"Runner", "Endless runner", "AtomOS", 500, 300, runner_init, NULL, NULL, NULL, NULL},
    {"Shooter", "Arcade shooter", "AtomOS", 400, 400, shooter_init, NULL, NULL, NULL, NULL},
    {"Platformer", "Jump game", "AtomOS", 500, 400, platformer_init, NULL, NULL, NULL, NULL},
    {"Racing", "Car racing", "AtomOS", 400, 500, racing_init, NULL, NULL, NULL, NULL},
    {"Hangman", "Word guessing", "AtomOS", 400, 300, hangman_init, NULL, NULL, NULL, NULL},
    {"Word Search", "Find words", "AtomOS", 400, 400, wordsearch_init, NULL, NULL, NULL, NULL},
    {"Typing", "Typing game", "AtomOS", 500, 300, typing_init, NULL, NULL, NULL, NULL},
    {"Anagram", "Word scramble", "AtomOS", 400, 300, anagram_init, NULL, NULL, NULL, NULL},
    {"Slots", "Slot machine", "AtomOS", 400, 300, slots_init, NULL, NULL, NULL, NULL},
    {"Roulette", "Casino wheel", "AtomOS", 400, 400, roulette_init, NULL, NULL, NULL, NULL},
    {"Dice", "Dice game", "AtomOS", 400, 300, dice_init, NULL, NULL, NULL, NULL},
    {"Golf", "Mini golf", "AtomOS", 500, 400, golf_init, NULL, NULL, NULL, NULL},
    {"Bowling", "10 pin bowling", "AtomOS", 400, 500, bowling_init, NULL, NULL, NULL, NULL},
    {"Darts", "Dart throwing", "AtomOS", 400, 400, darts_init, NULL, NULL, NULL, NULL},
    {"Pinball", "Pinball machine", "AtomOS", 400, 600, pinball_init, NULL, NULL, NULL, NULL},
    {"Air Hockey", "Table hockey", "AtomOS", 400, 500, airhockey_init, NULL, NULL, NULL, NULL},
    {"Pool", "Billiards", "AtomOS", 500, 350, pool_init, NULL, NULL, NULL, NULL},
    {"Tower Defense", "Strategy", "AtomOS", 600, 400, tower_defense_init, NULL, NULL, NULL, NULL},
    {"Clicker", "Idle game", "AtomOS", 400, 400, clicker_init, NULL, NULL, NULL, NULL},
    {"Simon", "Memory game", "AtomOS", 400, 400, simon_init, NULL, NULL, NULL, NULL},
    {"Quiz", "Trivia quiz", "AtomOS", 500, 400, quiz_init, NULL, NULL, NULL, NULL},
    {"Trivia", "General knowledge", "AtomOS", 500, 400, trivia_init, NULL, NULL, NULL, NULL},
};

int game_count = sizeof(game_list) / sizeof(game_list[0]);

/* Initialize game */
void game_init(game_info_t *info) {
    g_game = (game_ctx_t *)kcalloc(1, sizeof(game_ctx_t));
    if (!g_game) return;
    
    g_game->window = javier_window_create(
        info->name, 50, 50, info->min_width, info->min_height,
        WIN_DEFAULT
    );
    
    g_game->state = GAME_STATE_MENU;
    g_game->score = 0;
    g_game->high_score = 0;
    g_game->level = 1;
    g_game->lives = 3;
    g_game->running = true;
    g_game->last_update = timer_get_ticks();
    
    game_srand(timer_get_ticks());
    
    if (info->init) info->init();
}

/* Run game loop */
void game_run(void) {
    if (!g_game) return;
    
    while (g_game->running) {
        g_game->frame_count++;
        
        /* Process input */
        if (keyboard_has_key()) {
            key_event_t event = keyboard_get_key();
            if (event.pressed) {
                if (event.scancode == KEY_ESC) {
                    if (g_game->state == GAME_STATE_PLAYING) {
                        g_game->state = GAME_STATE_PAUSED;
                    } else if (g_game->state == GAME_STATE_PAUSED) {
                        g_game->state = GAME_STATE_PLAYING;
                    } else {
                        g_game->running = false;
                    }
                }
            }
        }
        
        timer_sleep_ms(16);  /* ~60 FPS */
    }
}

/* Quit game */
void game_quit(void) {
    if (g_game) {
        if (g_game->window) {
            javier_window_destroy(g_game->window);
        }
        kfree(g_game);
        g_game = NULL;
    }
}

/* Set game state */
void game_set_state(game_state_t state) {
    if (g_game) {
        g_game->state = state;
    }
}

/* Clear game area */
void game_clear(color_t color) {
    if (!g_game || !g_game->window) return;
    int x = g_game->window->bounds.x + g_game->window->client.x;
    int y = g_game->window->bounds.y + g_game->window->client.y;
    int w = g_game->window->client.width;
    int h = g_game->window->client.height;
    fb_fill_rect(x, y, w, h, color);
}

/* Draw text */
void game_draw_text(int x, int y, const char *text, color_t color) {
    if (!g_game || !g_game->window) return;
    int bx = g_game->window->bounds.x + g_game->window->client.x + x;
    int by = g_game->window->bounds.y + g_game->window->client.y + y;
    fb_draw_string(bx, by, text, color, 0);
}

/* Draw centered text */
void game_draw_text_centered(int y, const char *text, color_t color) {
    if (!g_game || !g_game->window) return;
    int len = strlen(text) * 8;
    int x = (g_game->window->client.width - len) / 2;
    game_draw_text(x, y, text, color);
}

/* Draw score */
void game_draw_score(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %d", g_game->score);
    game_draw_text(10, 10, buf, RGB(255, 255, 255));
}

/* Draw lives */
void game_draw_lives(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Lives: %d", g_game->lives);
    game_draw_text(g_game->window->client.width - 80, 10, buf, RGB(255, 255, 255));
}

/* Draw menu */
void game_draw_menu(const char *title, const char **options, int count, int selected) {
    game_clear(RGB(20, 20, 40));
    
    game_draw_text_centered(50, title, RGB(255, 200, 0));
    
    for (int i = 0; i < count; i++) {
        color_t color = (i == selected) ? RGB(255, 255, 255) : RGB(150, 150, 150);
        game_draw_text_centered(120 + i * 30, options[i], color);
        if (i == selected) {
            game_draw_text_centered(120 + i * 30 + 20, ">>>", RGB(255, 200, 0));
        }
    }
}

/* Draw game over */
void game_draw_gameover(void) {
    fb_fill_rect(g_game->window->bounds.x + g_game->window->client.x + 50,
                 g_game->window->bounds.y + g_game->window->client.y + 100,
                 g_game->window->client.width - 100, 100, RGBA(0, 0, 0, 200));
    
    game_draw_text_centered(120, "GAME OVER", RGB(255, 0, 0));
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Final Score: %d", g_game->score);
    game_draw_text_centered(150, buf, RGB(255, 255, 255));
    
    game_draw_text_centered(180, "Press ENTER to restart", RGB(200, 200, 200));
}

/* Draw pause */
void game_draw_pause(void) {
    fb_fill_rect(g_game->window->bounds.x + g_game->window->client.x + 50,
                 g_game->window->bounds.y + g_game->window->client.y + 100,
                 g_game->window->client.width - 100, 80, RGBA(0, 0, 0, 200));
    
    game_draw_text_centered(130, "PAUSED", RGB(255, 255, 0));
    game_draw_text_centered(160, "Press ESC to resume", RGB(200, 200, 200));
}

/* Random number generator */
void game_srand(uint32_t seed) {
    rand_state = seed;
}

uint32_t game_rand(void) {
    rand_state = rand_state * 1103515245 + 12345;
    return (rand_state >> 16) & 0x7FFF;
}

int game_rand_range(int min, int max) {
    return min + (game_rand() % (max - min + 1));
}

/* Collision detection */
bool game_rect_overlap(int x1, int y1, int w1, int h1,
                       int x2, int y2, int w2, int h2) {
    return !(x1 + w1 <= x2 || x2 + w2 <= x1 ||
             y1 + h1 <= y2 || y2 + h2 <= y1);
}

bool game_point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

int game_distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    /* Approximate sqrt */
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (dx > dy) ? dx + dy/2 : dy + dx/2;
}
