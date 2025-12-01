/*
 * AtomOS DAW (Digital Audio Workstation)
 * Complete audio production suite
 */

#ifndef _ATOMOS_DAW_H
#define _ATOMOS_DAW_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"
/* colors.h included via javier.h to avoid macro conflicts */

/* Audio settings */
#define DAW_SAMPLE_RATE 44100
#define DAW_BUFFER_SIZE 4096
#define DAW_MAX_TRACKS 16
#define DAW_MAX_PATTERNS 64
#define DAW_MAX_NOTES 256
#define DAW_MAX_EFFECTS 8
#define DAW_PATTERN_LEN 64

/* Note structure */
typedef struct {
    int note;       /* MIDI note (0-127) */
    int velocity;   /* Volume (0-127) */
    int start;      /* Start position in pattern */
    int duration;   /* Length in steps */
    bool active;
} daw_note_t;

/* Pattern structure */
typedef struct {
    char name[32];
    daw_note_t notes[DAW_MAX_NOTES];
    int note_count;
    int length;     /* In steps */
} daw_pattern_t;

/* Effect types */
typedef enum {
    FX_NONE = 0,
    FX_REVERB,
    FX_DELAY,
    FX_CHORUS,
    FX_DISTORTION,
    FX_FILTER,
    FX_COMPRESSOR,
    FX_EQ,
    FX_PHASER,
    FX_FLANGER
} daw_fx_type_t;

/* Effect structure */
typedef struct {
    daw_fx_type_t type;
    int params[8];
    bool enabled;
} daw_effect_t;

/* Instrument types */
typedef enum {
    INST_NONE = 0,
    INST_SINE,
    INST_SQUARE,
    INST_SAW,
    INST_TRIANGLE,
    INST_NOISE,
    INST_PIANO,
    INST_BASS,
    INST_PAD,
    INST_LEAD,
    INST_STRINGS,
    INST_BRASS,
    INST_DRUMS,
    INST_SAMPLER
} daw_inst_type_t;

/* Instrument structure */
typedef struct {
    char name[32];
    daw_inst_type_t type;
    int volume;
    int pan;
    int attack;
    int decay;
    int sustain;
    int release;
    int pitch;
    int detune;
    daw_effect_t effects[DAW_MAX_EFFECTS];
} daw_instrument_t;

/* Track structure */
typedef struct {
    char name[32];
    int volume;
    int pan;
    bool mute;
    bool solo;
    bool armed;
    int pattern_indices[DAW_MAX_PATTERNS]; /* Which patterns play at each position */
    daw_instrument_t instrument;
    daw_effect_t effects[DAW_MAX_EFFECTS];
    color_t color;
} daw_track_t;

/* Project structure */
typedef struct {
    char name[64];
    int bpm;
    int time_sig_num;   /* Time signature numerator */
    int time_sig_den;   /* Time signature denominator */
    int swing;          /* Swing amount */
    daw_track_t tracks[DAW_MAX_TRACKS];
    int track_count;
    daw_pattern_t patterns[DAW_MAX_PATTERNS];
    int pattern_count;
    int song_length;    /* In bars */
    int loop_start;
    int loop_end;
    bool loop_enabled;
} daw_project_t;

/* View modes */
typedef enum {
    VIEW_ARRANGE,       /* Song arrangement view */
    VIEW_PIANO_ROLL,    /* Piano roll editor */
    VIEW_MIXER,         /* Mixer view */
    VIEW_BROWSER,       /* File/instrument browser */
    VIEW_SETTINGS       /* Project settings */
} daw_view_t;

/* DAW state */
typedef struct {
    javier_window_t *window;
    daw_project_t project;
    daw_view_t view;
    bool playing;
    bool recording;
    int current_pos;    /* Playhead position */
    int current_bar;
    int current_beat;
    int current_tick;
    int selected_track;
    int selected_pattern;
    int selected_note;
    int zoom_x;
    int zoom_y;
    int scroll_x;
    int scroll_y;
    int octave;
    uint64_t last_tick;
    bool running;
} daw_state_t;

/* DAW functions */
daw_state_t *daw_create(void);
void daw_destroy(daw_state_t *daw);
void daw_new_project(daw_state_t *daw);
void daw_draw(daw_state_t *daw);
void daw_update(daw_state_t *daw);
void daw_play(daw_state_t *daw);
void daw_pause(daw_state_t *daw);
void daw_stop(daw_state_t *daw);
void daw_record(daw_state_t *daw);
void daw_seek(daw_state_t *daw, int position);

/* Track functions */
void daw_add_track(daw_state_t *daw, const char *name, daw_inst_type_t inst);
void daw_remove_track(daw_state_t *daw, int index);
void daw_mute_track(daw_state_t *daw, int index, bool mute);
void daw_solo_track(daw_state_t *daw, int index, bool solo);

/* Pattern functions */
daw_pattern_t *daw_create_pattern(daw_state_t *daw, const char *name);
void daw_add_note(daw_pattern_t *pattern, int note, int velocity, int start, int duration);
void daw_remove_note(daw_pattern_t *pattern, int index);

/* Mixer functions */
void daw_set_track_volume(daw_state_t *daw, int track, int volume);
void daw_set_track_pan(daw_state_t *daw, int track, int pan);
void daw_add_effect(daw_state_t *daw, int track, daw_fx_type_t type);

/* Audio synthesis */
int16_t daw_synth_sample(daw_instrument_t *inst, int note, int time, int env_time);
void daw_generate_audio(daw_state_t *daw, int16_t *buffer, int samples);

#endif /* _ATOMOS_DAW_H */
