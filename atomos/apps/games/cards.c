/*
 * AtomOS Card & Casino Games
 */

#include "games.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Card structure */
typedef struct {
    int suit;   /* 0-3: clubs, diamonds, hearts, spades */
    int rank;   /* 1-13: A, 2-10, J, Q, K */
    bool face_up;
} card_t;

/* Deck operations */
static void deck_init(card_t *deck, int *count) {
    *count = 52;
    int idx = 0;
    for (int s = 0; s < 4; s++) {
        for (int r = 1; r <= 13; r++) {
            deck[idx].suit = s;
            deck[idx].rank = r;
            deck[idx].face_up = false;
            idx++;
        }
    }
}

static void deck_shuffle(card_t *deck, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = game_rand() % (i + 1);
        card_t tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
}

/* ==================== SOLITAIRE ==================== */
#define SOL_TABLEAU 7
#define SOL_MAX_PILE 20

static struct {
    card_t tableau[SOL_TABLEAU][SOL_MAX_PILE];
    int tableau_count[SOL_TABLEAU];
    card_t foundation[4][13];
    int foundation_count[4];
    card_t stock[24];
    int stock_count;
    card_t waste[24];
    int waste_count;
    int cursor_pile;  /* 0-6 tableau, 7 stock, 8-11 foundation */
    int cursor_card;
    int held_pile;
    int held_card;
    bool has_held;
} solitaire;

void solitaire_init(void) {
    memset(&solitaire, 0, sizeof(solitaire));
    
    card_t deck[52];
    int deck_count;
    deck_init(deck, &deck_count);
    deck_shuffle(deck, deck_count);
    
    /* Deal to tableau */
    int idx = 0;
    for (int pile = 0; pile < SOL_TABLEAU; pile++) {
        for (int card = 0; card <= pile; card++) {
            solitaire.tableau[pile][card] = deck[idx++];
            solitaire.tableau_count[pile]++;
        }
        /* Flip top card */
        solitaire.tableau[pile][pile].face_up = true;
    }
    
    /* Rest to stock */
    while (idx < 52) {
        solitaire.stock[solitaire.stock_count++] = deck[idx++];
    }
}

/* ==================== BLACKJACK ==================== */
#define BJ_MAX_HAND 10

static struct {
    card_t player_hand[BJ_MAX_HAND];
    int player_count;
    card_t dealer_hand[BJ_MAX_HAND];
    int dealer_count;
    card_t deck[312];  /* 6 decks */
    int deck_pos;
    int deck_count;
    int player_money;
    int bet;
    int state;  /* 0=betting, 1=playing, 2=dealer, 3=result */
    bool player_bust;
    bool dealer_bust;
    bool player_blackjack;
    bool dealer_blackjack;
} blackjack;

/* Calculate blackjack hand value - exported for game updates */
__attribute__((unused))
static int blackjack_hand_value(card_t *hand, int count) {
    int value = 0;
    int aces = 0;
    for (int i = 0; i < count; i++) {
        int rank = hand[i].rank;
        if (rank == 1) {
            aces++;
            value += 11;
        } else if (rank >= 10) {
            value += 10;
        } else {
            value += rank;
        }
    }
    while (value > 21 && aces > 0) {
        value -= 10;
        aces--;
    }
    return value;
}

void blackjack_init(void) {
    memset(&blackjack, 0, sizeof(blackjack));
    blackjack.player_money = 1000;
    blackjack.bet = 10;
    
    /* Initialize 6-deck shoe */
    blackjack.deck_count = 0;
    for (int d = 0; d < 6; d++) {
        for (int s = 0; s < 4; s++) {
            for (int r = 1; r <= 13; r++) {
                blackjack.deck[blackjack.deck_count].suit = s;
                blackjack.deck[blackjack.deck_count].rank = r;
                blackjack.deck[blackjack.deck_count].face_up = true;
                blackjack.deck_count++;
            }
        }
    }
    deck_shuffle(blackjack.deck, blackjack.deck_count);
}

/* ==================== POKER ==================== */
#define POKER_PLAYERS 4

typedef enum {
    HAND_HIGH_CARD,
    HAND_PAIR,
    HAND_TWO_PAIR,
    HAND_THREE_OF_KIND,
    HAND_STRAIGHT,
    HAND_FLUSH,
    HAND_FULL_HOUSE,
    HAND_FOUR_OF_KIND,
    HAND_STRAIGHT_FLUSH,
    HAND_ROYAL_FLUSH
} poker_hand_t;

static struct {
    card_t player_hand[POKER_PLAYERS][2];
    card_t community[5];
    int community_count;
    card_t deck[52];
    int deck_pos;
    int player_chips[POKER_PLAYERS];
    int pot;
    int current_bet;
    int current_player;
    int dealer_pos;
    int state;  /* 0=preflop, 1=flop, 2=turn, 3=river, 4=showdown */
    bool folded[POKER_PLAYERS];
} poker;

void poker_init(void) {
    memset(&poker, 0, sizeof(poker));
    
    for (int i = 0; i < POKER_PLAYERS; i++) {
        poker.player_chips[i] = 1000;
    }
    
    deck_init(poker.deck, &poker.deck_pos);
    deck_shuffle(poker.deck, 52);
    poker.deck_pos = 0;
    
    /* Deal 2 cards to each player */
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < POKER_PLAYERS; p++) {
            poker.player_hand[p][c] = poker.deck[poker.deck_pos++];
            poker.player_hand[p][c].face_up = (p == 0);  /* Only show player's cards */
        }
    }
}

/* ==================== HEARTS ==================== */
static struct {
    card_t hands[4][13];
    int hand_counts[4];
    card_t trick[4];
    int trick_count;
    int lead_player;
    int current_player;
    int scores[4];
    int round_scores[4];
    bool hearts_broken;
} hearts;

void hearts_init(void) {
    memset(&hearts, 0, sizeof(hearts));
    
    card_t deck[52];
    int count;
    deck_init(deck, &count);
    deck_shuffle(deck, count);
    
    /* Deal 13 cards to each player */
    int idx = 0;
    for (int p = 0; p < 4; p++) {
        for (int c = 0; c < 13; c++) {
            hearts.hands[p][c] = deck[idx++];
            hearts.hands[p][c].face_up = (p == 0);
        }
        hearts.hand_counts[p] = 13;
    }
}

/* ==================== FREECELL ==================== */
static struct {
    card_t tableau[8][20];
    int tableau_count[8];
    card_t freecells[4];
    bool freecell_used[4];
    card_t foundation[4][13];
    int foundation_count[4];
    int cursor_col;
    int cursor_row;
} freecell;

void freecell_init(void) {
    memset(&freecell, 0, sizeof(freecell));
    
    card_t deck[52];
    int count;
    deck_init(deck, &count);
    deck_shuffle(deck, count);
    
    /* Deal to 8 columns */
    int idx = 0;
    for (int col = 0; col < 8; col++) {
        int cards = (col < 4) ? 7 : 6;
        for (int row = 0; row < cards; row++) {
            freecell.tableau[col][row] = deck[idx++];
            freecell.tableau[col][row].face_up = true;
            freecell.tableau_count[col]++;
        }
    }
}

/* ==================== SLOTS ==================== */
#define SLOT_REELS 3
#define SLOT_SYMBOLS 6

static struct {
    int reels[SLOT_REELS];
    int target[SLOT_REELS];
    int spinning[SLOT_REELS];
    int credits;
    int bet;
    int win;
    bool is_spinning;
} slots;

void slots_init(void) {
    memset(&slots, 0, sizeof(slots));
    slots.credits = 100;
    slots.bet = 1;
    for (int i = 0; i < SLOT_REELS; i++) {
        slots.reels[i] = game_rand() % SLOT_SYMBOLS;
    }
}

/* ==================== ROULETTE ==================== */
static struct {
    int ball_pos;
    int ball_speed;
    int wheel_offset;
    int bets[50];  /* Different bet positions */
    int bet_amounts[50];
    int credits;
    bool spinning;
    int result;
} roulette;

void roulette_init(void) {
    memset(&roulette, 0, sizeof(roulette));
    roulette.credits = 1000;
    roulette.result = -1;
}

/* ==================== DICE ==================== */
static struct {
    int dice[5];
    bool held[5];
    int rolls_left;
    int scores[13];
    bool scored[13];
    int total_score;
    int cursor;
} dice;

void dice_init(void) {
    memset(&dice, 0, sizeof(dice));
    dice.rolls_left = 3;
    for (int i = 0; i < 5; i++) {
        dice.dice[i] = 1 + game_rand() % 6;
    }
}
