/*
 * AtomOS Word Games
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Word list for games */
static const char *word_list[] = {
    "ATOMOS", "KERNEL", "SYSTEM", "MEMORY", "PROCESS",
    "DRIVER", "BUFFER", "THREAD", "CURSOR", "WINDOW",
    "SCREEN", "PIXEL", "FRAME", "INPUT", "OUTPUT",
    "JAVIER", "RENDER", "BITMAP", "SPRITE", "VECTOR",
    "MATRIX", "SHADER", "VERTEX", "NORMAL", "CAMERA",
    "LIGHT", "SHADOW", "TEXTURE", "BLEND", "FILTER",
    "AUDIO", "SAMPLE", "MIXER", "VOLUME", "TRACK",
    "NETWORK", "SOCKET", "PACKET", "SERVER", "CLIENT",
    "STORAGE", "SECTOR", "BLOCK", "CACHE", "QUEUE",
    "STACK", "HEAP", "LINKED", "BINARY", "SEARCH"
};
static const int word_count = sizeof(word_list) / sizeof(word_list[0]);

/* ==================== HANGMAN ==================== */
#define HANGMAN_MAX_WRONG 6

static struct {
    char word[32];
    char guessed[32];
    bool tried[26];
    int wrong_guesses;
    int letters_found;
    int letters_total;
    bool won;
    bool lost;
    char last_guess;
} hangman;

void hangman_init(void) {
    memset(&hangman, 0, sizeof(hangman));
    
    /* Pick random word */
    const char *word = word_list[game_rand() % word_count];
    strcpy(hangman.word, word);
    hangman.letters_total = strlen(word);
    
    /* Initialize guessed with underscores */
    for (int i = 0; i < hangman.letters_total; i++) {
        hangman.guessed[i] = '_';
    }
    hangman.guessed[hangman.letters_total] = '\0';
}

/* ==================== WORD SEARCH ==================== */
#define WORDSEARCH_SIZE 12
#define WORDSEARCH_WORDS 8

static struct {
    char grid[WORDSEARCH_SIZE][WORDSEARCH_SIZE + 1];
    char words[WORDSEARCH_WORDS][16];
    bool found[WORDSEARCH_WORDS];
    int cursor_x, cursor_y;
    int select_start_x, select_start_y;
    bool selecting;
    int words_found;
} wordsearch;

static void wordsearch_place_word(const char *word) {
    int len = strlen(word);
    int attempts = 50;
    
    while (attempts-- > 0) {
        int dir = game_rand() % 8;
        int dx[] = {1, 1, 0, -1, -1, -1, 0, 1};
        int dy[] = {0, 1, 1, 1, 0, -1, -1, -1};
        
        int x = game_rand() % WORDSEARCH_SIZE;
        int y = game_rand() % WORDSEARCH_SIZE;
        
        /* Check if word fits */
        int ex = x + dx[dir] * (len - 1);
        int ey = y + dy[dir] * (len - 1);
        if (ex < 0 || ex >= WORDSEARCH_SIZE || ey < 0 || ey >= WORDSEARCH_SIZE) {
            continue;
        }
        
        /* Check if space is available */
        bool fits = true;
        for (int i = 0; i < len && fits; i++) {
            int px = x + dx[dir] * i;
            int py = y + dy[dir] * i;
            char c = wordsearch.grid[py][px];
            if (c != ' ' && c != word[i]) {
                fits = false;
            }
        }
        
        if (fits) {
            for (int i = 0; i < len; i++) {
                int px = x + dx[dir] * i;
                int py = y + dy[dir] * i;
                wordsearch.grid[py][px] = word[i];
            }
            return;
        }
    }
}

void wordsearch_init(void) {
    memset(&wordsearch, 0, sizeof(wordsearch));
    
    /* Fill with spaces */
    for (int y = 0; y < WORDSEARCH_SIZE; y++) {
        for (int x = 0; x < WORDSEARCH_SIZE; x++) {
            wordsearch.grid[y][x] = ' ';
        }
        wordsearch.grid[y][WORDSEARCH_SIZE] = '\0';
    }
    
    /* Pick and place words */
    int used[50] = {0};
    for (int i = 0; i < WORDSEARCH_WORDS; i++) {
        int idx;
        do {
            idx = game_rand() % word_count;
        } while (used[idx] || strlen(word_list[idx]) > 8);
        used[idx] = 1;
        
        strcpy(wordsearch.words[i], word_list[idx]);
        wordsearch_place_word(word_list[idx]);
    }
    
    /* Fill remaining with random letters */
    for (int y = 0; y < WORDSEARCH_SIZE; y++) {
        for (int x = 0; x < WORDSEARCH_SIZE; x++) {
            if (wordsearch.grid[y][x] == ' ') {
                wordsearch.grid[y][x] = 'A' + game_rand() % 26;
            }
        }
    }
}

/* ==================== TYPING GAME ==================== */
#define TYPING_WORDS 5

static struct {
    char current_word[32];
    char typed[32];
    int typed_len;
    int words_completed;
    int words_missed;
    int wpm;
    uint64_t start_time;
    int falling_words_x[TYPING_WORDS];
    int falling_words_y[TYPING_WORDS];
    char falling_words[TYPING_WORDS][16];
    bool active[TYPING_WORDS];
} typing;

void typing_init(void) {
    memset(&typing, 0, sizeof(typing));
    typing.start_time = timer_get_ticks();
    
    /* Spawn initial words */
    for (int i = 0; i < TYPING_WORDS; i++) {
        typing.falling_words_x[i] = game_rand_range(20, 400);
        typing.falling_words_y[i] = -game_rand_range(0, 200);
        strcpy(typing.falling_words[i], word_list[game_rand() % word_count]);
        typing.active[i] = true;
    }
}

/* ==================== ANAGRAM ==================== */
static struct {
    char original[32];
    char scrambled[32];
    char answer[32];
    int answer_len;
    int words_solved;
    int cursor;
} anagram;

static void scramble_word(const char *src, char *dst) {
    int len = strlen(src);
    strcpy(dst, src);
    for (int i = len - 1; i > 0; i--) {
        int j = game_rand() % (i + 1);
        char tmp = dst[i];
        dst[i] = dst[j];
        dst[j] = tmp;
    }
}

void anagram_init(void) {
    memset(&anagram, 0, sizeof(anagram));
    
    const char *word = word_list[game_rand() % word_count];
    strcpy(anagram.original, word);
    scramble_word(word, anagram.scrambled);
}

/* ==================== SIMON ==================== */
#define SIMON_MAX_SEQ 50

static struct {
    int sequence[SIMON_MAX_SEQ];
    int seq_len;
    int player_pos;
    int state;  /* 0=showing, 1=input, 2=fail */
    int show_pos;
    int show_timer;
    int highlighted;
} simon;

void simon_init(void) {
    memset(&simon, 0, sizeof(simon));
    simon.seq_len = 1;
    simon.sequence[0] = game_rand() % 4;
    simon.state = 0;
    simon.highlighted = -1;
}

/* ==================== TOWER DEFENSE ==================== */
#define TD_MAP_W 20
#define TD_MAP_H 12
#define TD_MAX_ENEMIES 30
#define TD_MAX_TOWERS 20
#define TD_MAX_BULLETS 50

typedef struct {
    int x, y;
    int health;
    int max_health;
    int speed;
    int path_pos;
    bool active;
} td_enemy_t;

typedef struct {
    int x, y;
    int type;
    int level;
    int range;
    int damage;
    int fire_rate;
    int fire_timer;
    bool active;
} td_tower_t;

typedef struct {
    int x, y;
    int target_idx;
    int damage;
    bool active;
} td_bullet_t;

static struct {
    int map[TD_MAP_H][TD_MAP_W];
    int path_x[100], path_y[100];
    int path_len;
    td_enemy_t enemies[TD_MAX_ENEMIES];
    td_tower_t towers[TD_MAX_TOWERS];
    td_bullet_t bullets[TD_MAX_BULLETS];
    int money;
    int lives;
    int wave;
    int spawn_timer;
    int enemies_to_spawn;
    int cursor_x, cursor_y;
    int selected_tower_type;
    bool placing;
} td;

void tower_defense_init(void) {
    memset(&td, 0, sizeof(td));
    td.money = 200;
    td.lives = 20;
    td.wave = 1;
    td.enemies_to_spawn = 10;
    
    /* Create simple path */
    td.path_len = 0;
    for (int x = 0; x < TD_MAP_W; x++) {
        td.path_x[td.path_len] = x;
        td.path_y[td.path_len] = TD_MAP_H / 2;
        td.map[TD_MAP_H / 2][x] = 1;  /* Path tile */
        td.path_len++;
    }
}

/* ==================== CLICKER ==================== */
static struct {
    uint64_t clicks;
    uint64_t total_clicks;
    uint64_t auto_clickers;
    uint64_t auto_rate;
    uint64_t multipliers;
    int upgrades[10];
    int upgrade_costs[10];
    uint64_t last_auto;
} clicker;

void clicker_init(void) {
    memset(&clicker, 0, sizeof(clicker));
    clicker.auto_rate = 1;
    
    /* Initialize upgrade costs */
    clicker.upgrade_costs[0] = 10;   /* Auto clicker */
    clicker.upgrade_costs[1] = 50;   /* Click multiplier */
    clicker.upgrade_costs[2] = 100;  /* Auto rate boost */
    clicker.upgrade_costs[3] = 500;
    clicker.upgrade_costs[4] = 1000;
}

/* ==================== QUIZ ==================== */
typedef struct {
    const char *question;
    const char *answers[4];
    int correct;
} quiz_question_t;

static const quiz_question_t quiz_questions[] = {
    {"What is the capital of France?", {"London", "Paris", "Berlin", "Madrid"}, 1},
    {"What year did WWII end?", {"1943", "1944", "1945", "1946"}, 2},
    {"What is 7 x 8?", {"54", "56", "48", "64"}, 1},
    {"Which planet is closest to the Sun?", {"Venus", "Earth", "Mercury", "Mars"}, 2},
    {"Who wrote Romeo and Juliet?", {"Dickens", "Shakespeare", "Austen", "Twain"}, 1},
    {"What is H2O?", {"Oxygen", "Hydrogen", "Water", "Carbon"}, 2},
    {"How many continents are there?", {"5", "6", "7", "8"}, 2},
    {"What is the largest ocean?", {"Atlantic", "Indian", "Arctic", "Pacific"}, 3},
    {"Which gas do plants absorb?", {"Oxygen", "CO2", "Nitrogen", "Hydrogen"}, 1},
    {"What year did the Berlin Wall fall?", {"1987", "1988", "1989", "1990"}, 2},
};

static struct {
    int current_question;
    int selected_answer;
    int score;
    int questions_answered;
    bool answered;
    bool correct;
} quiz;

void quiz_init(void) {
    memset(&quiz, 0, sizeof(quiz));
    quiz.current_question = game_rand() % (sizeof(quiz_questions) / sizeof(quiz_questions[0]));
}

/* ==================== TRIVIA ==================== */
void trivia_init(void) {
    quiz_init();  /* Same as quiz */
}
