/*
 * AtomOS Puzzle Games
 * Puzzle and strategy game implementations
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* ==================== MINESWEEPER ==================== */
#define MINE_W 16
#define MINE_H 16
#define MINE_COUNT 40

static struct {
    int grid[MINE_H][MINE_W];      /* -1 = mine, 0-8 = count */
    int revealed[MINE_H][MINE_W];  /* 0 = hidden, 1 = revealed, 2 = flagged */
    int cursor_x, cursor_y;
    int mines_flagged;
    int cells_revealed;
    bool game_over;
    bool first_click;
} minesweeper;

void minesweeper_init(void) {
    memset(&minesweeper, 0, sizeof(minesweeper));
    minesweeper.cursor_x = MINE_W / 2;
    minesweeper.cursor_y = MINE_H / 2;
    minesweeper.first_click = true;
    
    /* Place mines */
    int placed = 0;
    while (placed < MINE_COUNT) {
        int x = game_rand() % MINE_W;
        int y = game_rand() % MINE_H;
        if (minesweeper.grid[y][x] != -1) {
            minesweeper.grid[y][x] = -1;
            placed++;
        }
    }
    
    /* Calculate numbers */
    for (int y = 0; y < MINE_H; y++) {
        for (int x = 0; x < MINE_W; x++) {
            if (minesweeper.grid[y][x] == -1) continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < MINE_W && ny >= 0 && ny < MINE_H) {
                        if (minesweeper.grid[ny][nx] == -1) count++;
                    }
                }
            }
            minesweeper.grid[y][x] = count;
        }
    }
}

/* ==================== SUDOKU ==================== */
static struct {
    int grid[9][9];
    int solution[9][9];
    int fixed[9][9];
    int cursor_x, cursor_y;
    int selected_num;
    bool completed;
} sudoku;

static void sudoku_generate(void) {
    /* Simple sudoku generator - create a valid solution first */
    memset(sudoku.grid, 0, sizeof(sudoku.grid));
    memset(sudoku.fixed, 0, sizeof(sudoku.fixed));
    
    /* Fill diagonal 3x3 boxes first (they don't affect each other) */
    for (int box = 0; box < 3; box++) {
        int nums[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        /* Shuffle */
        for (int i = 8; i > 0; i--) {
            int j = game_rand() % (i + 1);
            int tmp = nums[i];
            nums[i] = nums[j];
            nums[j] = tmp;
        }
        int idx = 0;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                sudoku.grid[box * 3 + r][box * 3 + c] = nums[idx++];
            }
        }
    }
    
    /* Copy to solution */
    memcpy(sudoku.solution, sudoku.grid, sizeof(sudoku.grid));
    
    /* Remove some numbers (keep ~35 clues) */
    int remove = 81 - 35;
    while (remove > 0) {
        int r = game_rand() % 9;
        int c = game_rand() % 9;
        if (sudoku.grid[r][c] != 0) {
            sudoku.grid[r][c] = 0;
            remove--;
        }
    }
    
    /* Mark fixed cells */
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            sudoku.fixed[r][c] = (sudoku.grid[r][c] != 0);
        }
    }
}

void sudoku_init(void) {
    memset(&sudoku, 0, sizeof(sudoku));
    sudoku.cursor_x = 4;
    sudoku.cursor_y = 4;
    sudoku.selected_num = 1;
    sudoku_generate();
}

/* ==================== 2048 ==================== */
static struct {
    int grid[4][4];
    bool moved;
    int best_tile;
} game2048_state;

static void game2048_spawn(void) {
    int empty[16][2];
    int count = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (game2048_state.grid[r][c] == 0) {
                empty[count][0] = r;
                empty[count][1] = c;
                count++;
            }
        }
    }
    if (count > 0) {
        int idx = game_rand() % count;
        int val = (game_rand() % 10 < 9) ? 2 : 4;
        game2048_state.grid[empty[idx][0]][empty[idx][1]] = val;
    }
}

void game2048_init(void) {
    memset(&game2048_state, 0, sizeof(game2048_state));
    game2048_spawn();
    game2048_spawn();
}

/* ==================== MEMORY ==================== */
#define MEMORY_PAIRS 8

static struct {
    int cards[4][4];
    int revealed[4][4];
    int first_x, first_y;
    int second_x, second_y;
    int cursor_x, cursor_y;
    int pairs_found;
    int state;  /* 0 = picking first, 1 = picking second, 2 = showing */
    int show_timer;
} memory;

void memory_init(void) {
    memset(&memory, 0, sizeof(memory));
    memory.first_x = memory.first_y = -1;
    memory.second_x = memory.second_y = -1;
    
    /* Create pairs */
    int values[16];
    for (int i = 0; i < 8; i++) {
        values[i * 2] = i + 1;
        values[i * 2 + 1] = i + 1;
    }
    
    /* Shuffle */
    for (int i = 15; i > 0; i--) {
        int j = game_rand() % (i + 1);
        int tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }
    
    /* Place on grid */
    int idx = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            memory.cards[r][c] = values[idx++];
        }
    }
}

/* ==================== SLIDING PUZZLE ==================== */
static struct {
    int grid[4][4];
    int empty_x, empty_y;
    int moves;
    bool solved;
} puzzle;

void puzzle_init(void) {
    /* Create solved state */
    int n = 1;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            puzzle.grid[r][c] = n++;
        }
    }
    puzzle.grid[3][3] = 0;
    puzzle.empty_x = 3;
    puzzle.empty_y = 3;
    
    /* Shuffle with valid moves */
    for (int i = 0; i < 100; i++) {
        int dir = game_rand() % 4;
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        int nx = puzzle.empty_x + dx[dir];
        int ny = puzzle.empty_y + dy[dir];
        if (nx >= 0 && nx < 4 && ny >= 0 && ny < 4) {
            puzzle.grid[puzzle.empty_y][puzzle.empty_x] = puzzle.grid[ny][nx];
            puzzle.grid[ny][nx] = 0;
            puzzle.empty_x = nx;
            puzzle.empty_y = ny;
        }
    }
    puzzle.moves = 0;
    puzzle.solved = false;
}

/* ==================== MATCH 3 ==================== */
#define MATCH3_W 8
#define MATCH3_H 8
#define MATCH3_COLORS 6

static struct {
    int grid[MATCH3_H][MATCH3_W];
    int cursor_x, cursor_y;
    int selected_x, selected_y;
    bool has_selection;
    int combo;
} match3;

void match3_init(void) {
    memset(&match3, 0, sizeof(match3));
    match3.selected_x = match3.selected_y = -1;
    
    /* Fill grid with random colors */
    for (int r = 0; r < MATCH3_H; r++) {
        for (int c = 0; c < MATCH3_W; c++) {
            match3.grid[r][c] = 1 + game_rand() % MATCH3_COLORS;
        }
    }
}

/* ==================== SOKOBAN ==================== */
#define SOKOBAN_W 12
#define SOKOBAN_H 10

static struct {
    int level[SOKOBAN_H][SOKOBAN_W];  /* 0=floor, 1=wall, 2=target */
    int player_x, player_y;
    int boxes[10][2];
    int box_count;
    int level_num;
    int pushes;
} sokoban;

/* Simple level */
static const int sokoban_level1[SOKOBAN_H][SOKOBAN_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,2,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,2,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,2,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1},
};

void sokoban_init(void) {
    memset(&sokoban, 0, sizeof(sokoban));
    sokoban.level_num = 1;
    
    /* Copy level */
    memcpy(sokoban.level, sokoban_level1, sizeof(sokoban_level1));
    
    sokoban.player_x = 2;
    sokoban.player_y = 5;
    
    /* Place boxes */
    sokoban.box_count = 3;
    sokoban.boxes[0][0] = 4; sokoban.boxes[0][1] = 3;
    sokoban.boxes[1][0] = 4; sokoban.boxes[1][1] = 5;
    sokoban.boxes[2][0] = 4; sokoban.boxes[2][1] = 7;
}

/* ==================== 15 PUZZLE ==================== */
void fifteen_init(void) {
    puzzle_init();  /* Same as sliding puzzle */
}

/* ==================== CHESS ==================== */
#define CHESS_EMPTY 0
#define CHESS_PAWN 1
#define CHESS_KNIGHT 2
#define CHESS_BISHOP 3
#define CHESS_ROOK 4
#define CHESS_QUEEN 5
#define CHESS_KING 6

static struct {
    int board[8][8];  /* positive = white, negative = black */
    int cursor_x, cursor_y;
    int selected_x, selected_y;
    bool has_selection;
    bool white_turn;
    bool check;
    bool checkmate;
} chess;

void chess_init(void) {
    memset(&chess, 0, sizeof(chess));
    chess.cursor_x = 4;
    chess.cursor_y = 6;
    chess.white_turn = true;
    
    /* Set up pieces */
    int back_row[] = {CHESS_ROOK, CHESS_KNIGHT, CHESS_BISHOP, CHESS_QUEEN,
                      CHESS_KING, CHESS_BISHOP, CHESS_KNIGHT, CHESS_ROOK};
    
    for (int c = 0; c < 8; c++) {
        chess.board[0][c] = -back_row[c];   /* Black back row */
        chess.board[1][c] = -CHESS_PAWN;    /* Black pawns */
        chess.board[6][c] = CHESS_PAWN;     /* White pawns */
        chess.board[7][c] = back_row[c];    /* White back row */
    }
}

/* ==================== CHECKERS ==================== */
static struct {
    int board[8][8];  /* 1/-1 = piece, 2/-2 = king */
    int cursor_x, cursor_y;
    int selected_x, selected_y;
    bool has_selection;
    bool red_turn;
} checkers;

void checkers_init(void) {
    memset(&checkers, 0, sizeof(checkers));
    checkers.cursor_x = 0;
    checkers.cursor_y = 5;
    checkers.red_turn = true;
    
    /* Set up pieces */
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 8; c++) {
            if ((r + c) % 2 == 1) {
                checkers.board[r][c] = -1;  /* Black */
            }
        }
    }
    for (int r = 5; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if ((r + c) % 2 == 1) {
                checkers.board[r][c] = 1;   /* Red */
            }
        }
    }
}

/* ==================== TIC-TAC-TOE ==================== */
static struct {
    int board[3][3];  /* 0=empty, 1=X, 2=O */
    int cursor_x, cursor_y;
    bool x_turn;
    int winner;  /* 0=none, 1=X, 2=O, 3=draw */
} tictactoe;

void tictactoe_init(void) {
    memset(&tictactoe, 0, sizeof(tictactoe));
    tictactoe.cursor_x = 1;
    tictactoe.cursor_y = 1;
    tictactoe.x_turn = true;
}

/* ==================== CONNECT 4 ==================== */
static struct {
    int board[6][7];  /* 0=empty, 1=red, 2=yellow */
    int cursor_x;
    bool red_turn;
    int winner;
} connect4;

void connect4_init(void) {
    memset(&connect4, 0, sizeof(connect4));
    connect4.cursor_x = 3;
    connect4.red_turn = true;
}

/* ==================== REVERSI ==================== */
static struct {
    int board[8][8];  /* 0=empty, 1=black, 2=white */
    int cursor_x, cursor_y;
    bool black_turn;
    int black_count, white_count;
} reversi;

void reversi_init(void) {
    memset(&reversi, 0, sizeof(reversi));
    reversi.cursor_x = 3;
    reversi.cursor_y = 2;
    reversi.black_turn = true;
    
    /* Starting position */
    reversi.board[3][3] = 2;
    reversi.board[4][4] = 2;
    reversi.board[3][4] = 1;
    reversi.board[4][3] = 1;
    reversi.black_count = 2;
    reversi.white_count = 2;
}

/* ==================== GOMOKU ==================== */
static struct {
    int board[15][15];
    int cursor_x, cursor_y;
    bool black_turn;
    int winner;
} gomoku;

void gomoku_init(void) {
    memset(&gomoku, 0, sizeof(gomoku));
    gomoku.cursor_x = 7;
    gomoku.cursor_y = 7;
    gomoku.black_turn = true;
}
