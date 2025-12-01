/*
 * Javier Music Player
 * PC Speaker and basic audio support
 */

#ifndef _JAVIER_MUSIC_H
#define _JAVIER_MUSIC_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* Audio formats */
typedef enum {
    AUDIO_FORMAT_UNKNOWN,
    AUDIO_FORMAT_WAV,
    AUDIO_FORMAT_BEEP,      /* Simple frequency/duration sequences */
    AUDIO_FORMAT_MOD        /* Tracker module */
} audio_format_t;

/* Audio sample */
typedef struct {
    int16_t *samples;
    uint32_t sample_count;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
} audio_sample_t;

/* Track info */
typedef struct {
    char title[64];
    char artist[64];
    char album[64];
    uint32_t duration_ms;
    audio_format_t format;
} track_info_t;

/* Playlist entry */
typedef struct playlist_entry {
    char path[256];
    track_info_t info;
    struct playlist_entry *next;
    struct playlist_entry *prev;
} playlist_entry_t;

/* Player state */
typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} player_state_t;

/* Music player */
typedef struct {
    javier_window_t *window;
    playlist_entry_t *playlist;
    playlist_entry_t *current_track;
    player_state_t state;
    uint32_t position_ms;
    uint8_t volume;         /* 0-100 */
    bool repeat;
    bool shuffle;
    bool running;
} music_player_t;

/* PC Speaker driver */
void speaker_init(void);
void speaker_beep(uint32_t frequency, uint32_t duration_ms);
void speaker_off(void);
void speaker_set_frequency(uint32_t frequency);

/* Music notes (frequencies in Hz) */
#define NOTE_C3     131
#define NOTE_D3     147
#define NOTE_E3     165
#define NOTE_F3     175
#define NOTE_G3     196
#define NOTE_A3     220
#define NOTE_B3     247
#define NOTE_C4     262
#define NOTE_D4     294
#define NOTE_E4     330
#define NOTE_F4     349
#define NOTE_G4     392
#define NOTE_A4     440
#define NOTE_B4     494
#define NOTE_C5     523
#define NOTE_D5     587
#define NOTE_E5     659
#define NOTE_F5     698
#define NOTE_G5     784
#define NOTE_A5     880
#define NOTE_B5     988
#define NOTE_C6     1047
#define NOTE_REST   0

/* Note duration (relative to quarter note = 400ms) */
#define DUR_WHOLE       1600
#define DUR_HALF        800
#define DUR_QUARTER     400
#define DUR_EIGHTH      200
#define DUR_SIXTEENTH   100

/* Melody note */
typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} melody_note_t;

/* Predefined melodies */
extern const melody_note_t melody_startup[];
extern const melody_note_t melody_error[];
extern const melody_note_t melody_success[];
extern const melody_note_t melody_notification[];

/* Play melody */
void music_play_melody(const melody_note_t *melody, int note_count);

/* Player functions */
music_player_t *music_player_create(int x, int y, int width, int height);
void music_player_destroy(music_player_t *player);
void music_player_draw(music_player_t *player);
void music_player_update(music_player_t *player);

/* Playback control */
void music_player_play(music_player_t *player);
void music_player_pause(music_player_t *player);
void music_player_stop(music_player_t *player);
void music_player_next(music_player_t *player);
void music_player_prev(music_player_t *player);
void music_player_seek(music_player_t *player, uint32_t position_ms);
void music_player_set_volume(music_player_t *player, uint8_t volume);

/* Playlist management */
void music_player_add_track(music_player_t *player, const char *path);
void music_player_remove_track(music_player_t *player, playlist_entry_t *entry);
void music_player_clear_playlist(music_player_t *player);
void music_player_load_playlist(music_player_t *player, const char *path);
void music_player_save_playlist(music_player_t *player, const char *path);

#endif /* _JAVIER_MUSIC_H */
