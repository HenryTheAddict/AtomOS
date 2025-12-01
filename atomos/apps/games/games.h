/*
 * AtomOS Game Framework
 * Core game engine for all Javier games
 */

#ifndef _ATOMOS_GAMES_H
#define _ATOMOS_GAMES_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"

/* Game state */
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAMEOVER,
    GAME_STATE_WIN
} game_state_t;

/* Game info */
typedef struct {
    const char *name;
    const char *description;
    const char *author;
    int min_width;
    int min_height;
    void (*init)(void);
    void (*update)(void);
    void (*draw)(void);
    void (*input)(key_event_t *event);
    void (*cleanup)(void);
} game_info_t;

/* Game context */
typedef struct {
    javier_window_t *window;
    game_state_t state;
    int score;
    int high_score;
    int level;
    int lives;
    uint64_t frame_count;
    uint64_t last_update;
    bool running;
} game_ctx_t;

/* Current game context */
extern game_ctx_t *g_game;

/* Game framework */
void game_init(game_info_t *info);
void game_run(void);
void game_quit(void);
void game_set_state(game_state_t state);

/* Drawing helpers */
void game_clear(color_t color);
void game_draw_text(int x, int y, const char *text, color_t color);
void game_draw_text_centered(int y, const char *text, color_t color);
void game_draw_score(void);
void game_draw_lives(void);
void game_draw_menu(const char *title, const char **options, int count, int selected);
void game_draw_gameover(void);
void game_draw_pause(void);

/* Random number generator */
void game_srand(uint32_t seed);
uint32_t game_rand(void);
int game_rand_range(int min, int max);

/* Collision detection */
bool game_rect_overlap(int x1, int y1, int w1, int h1, 
                       int x2, int y2, int w2, int h2);
bool game_point_in_rect(int px, int py, int x, int y, int w, int h);
int game_distance(int x1, int y1, int x2, int y2);

/* ============ GAME DECLARATIONS ============ */

/* Classic Arcade */
void snake_init(void);
void tetris_init(void);
void pong_init(void);
void breakout_init(void);
void space_invaders_init(void);
void asteroids_init(void);
void frogger_init(void);
void pacman_init(void);

/* Puzzle Games */
void minesweeper_init(void);
void sudoku_init(void);
void game2048_init(void);
void memory_init(void);
void puzzle_init(void);
void match3_init(void);
void sokoban_init(void);
void fifteen_init(void);

/* Card Games */
void solitaire_init(void);
void blackjack_init(void);
void poker_init(void);
void hearts_init(void);
void freecell_init(void);

/* Board Games */
void chess_init(void);
void checkers_init(void);
void tictactoe_init(void);
void connect4_init(void);
void reversi_init(void);
void gomoku_init(void);

/* Action Games */
void flappy_init(void);
void runner_init(void);
void shooter_init(void);
void platformer_init(void);
void racing_init(void);

/* Word Games */
void hangman_init(void);
void wordsearch_init(void);
void typing_init(void);
void anagram_init(void);

/* Casino Games */
void slots_init(void);
void roulette_init(void);
void dice_init(void);

/* Sports Games */
void golf_init(void);
void bowling_init(void);
void darts_init(void);
void pinball_init(void);
void airhockey_init(void);
void pool_init(void);

/* Strategy Games */
void tower_defense_init(void);
void clicker_init(void);
void simon_init(void);

/* Quiz & Trivia */
void quiz_init(void);
void trivia_init(void);

/* List of all games */
extern game_info_t game_list[];
extern int game_count;

#endif /* _ATOMOS_GAMES_H */
