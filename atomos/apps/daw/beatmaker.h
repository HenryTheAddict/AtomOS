/*
 * AtomOS Beat Maker
 * Simple drum machine and beat creation tool
 */

#ifndef _ATOMOS_BEATMAKER_H
#define _ATOMOS_BEATMAKER_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* Beat maker settings */
#define BM_TRACKS 8
#define BM_STEPS 16
#define BM_PATTERNS 16

/* Drum sounds */
typedef enum {
    DRUM_KICK,
    DRUM_SNARE,
    DRUM_HIHAT_CLOSED,
    DRUM_HIHAT_OPEN,
    DRUM_TOM_HIGH,
    DRUM_TOM_MID,
    DRUM_TOM_LOW,
    DRUM_CLAP,
    DRUM_CRASH,
    DRUM_RIDE,
    DRUM_COWBELL,
    DRUM_SHAKER,
    DRUM_COUNT
} drum_type_t;

/* Beat pattern */
typedef struct {
    char name[32];
    bool steps[BM_TRACKS][BM_STEPS];
    int velocities[BM_TRACKS][BM_STEPS];
    int swing;
} beat_pattern_t;

/* Beat maker state */
typedef struct {
    javier_window_t *window;
    beat_pattern_t patterns[BM_PATTERNS];
    int current_pattern;
    int bpm;
    bool playing;
    int current_step;
    int cursor_x;
    int cursor_y;
    drum_type_t drum_sounds[BM_TRACKS];
    int track_volumes[BM_TRACKS];
    bool track_muted[BM_TRACKS];
    bool track_solo[BM_TRACKS];
    uint64_t last_step_time;
    bool running;
} beatmaker_t;

/* Beat maker functions */
beatmaker_t *beatmaker_create(void);
void beatmaker_destroy(beatmaker_t *bm);
void beatmaker_draw(beatmaker_t *bm);
void beatmaker_update(beatmaker_t *bm);
void beatmaker_play(beatmaker_t *bm);
void beatmaker_stop(beatmaker_t *bm);
void beatmaker_toggle_step(beatmaker_t *bm, int track, int step);
void beatmaker_clear_pattern(beatmaker_t *bm);
void beatmaker_copy_pattern(beatmaker_t *bm, int from, int to);
void beatmaker_trigger_sound(beatmaker_t *bm, drum_type_t drum, int velocity);

/* Preset patterns */
void beatmaker_load_preset(beatmaker_t *bm, int preset);

#endif /* _ATOMOS_BEATMAKER_H */
