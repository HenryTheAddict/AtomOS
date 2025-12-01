/*
 * Javier Music Player Implementation
 * PC Speaker audio and melody playback
 */

#include "music.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/fs/vfs.h"
#include "../../kernel/drivers/timer.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"

/* PC Speaker I/O ports */
#define SPEAKER_PORT        0x61
#define PIT_CHANNEL2_PORT   0x42
#define PIT_COMMAND_PORT    0x43

/* PIT frequency */
#define PIT_FREQUENCY       1193182

/* Predefined melodies */
const melody_note_t melody_startup[] = {
    {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_C6, 200}
};

const melody_note_t melody_error[] = {
    {NOTE_A4, 100}, {NOTE_REST, 50}, {NOTE_A4, 100}, {NOTE_REST, 50},
    {NOTE_A4, 200}
};

const melody_note_t melody_success[] = {
    {NOTE_G4, 100}, {NOTE_C5, 100}, {NOTE_E5, 200}
};

const melody_note_t melody_notification[] = {
    {NOTE_E5, 100}, {NOTE_REST, 50}, {NOTE_E5, 100}
};

/*
 * Initialize PC speaker
 */
void speaker_init(void) {
    /* Set PIT channel 2 to mode 3 (square wave) */
    outb(PIT_COMMAND_PORT, 0xB6);
}

/*
 * Set speaker frequency
 */
void speaker_set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        speaker_off();
        return;
    }
    
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Set frequency */
    outb(PIT_CHANNEL2_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL2_PORT, (uint8_t)((divisor >> 8) & 0xFF));
    
    /* Enable speaker */
    uint8_t current = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current | 0x03);
}

/*
 * Turn off speaker
 */
void speaker_off(void) {
    uint8_t current = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current & 0xFC);
}

/*
 * Play a beep
 */
void speaker_beep(uint32_t frequency, uint32_t duration_ms) {
    if (frequency == 0 || frequency == NOTE_REST) {
        timer_sleep_ms(duration_ms);
        return;
    }
    
    speaker_set_frequency(frequency);
    timer_sleep_ms(duration_ms);
    speaker_off();
}

/*
 * Play a melody
 */
void music_play_melody(const melody_note_t *melody, int note_count) {
    for (int i = 0; i < note_count; i++) {
        speaker_beep(melody[i].frequency, melody[i].duration_ms);
        timer_sleep_ms(10);  /* Small gap between notes */
    }
}

/*
 * Create music player
 */
music_player_t *music_player_create(int x, int y, int width, int height) {
    music_player_t *player = (music_player_t *)kcalloc(1, sizeof(music_player_t));
    if (!player) return NULL;
    
    player->window = javier_window_create("Music Player", x, y, width, height,
                                           WIN_DEFAULT);
    if (!player->window) {
        kfree(player);
        return NULL;
    }
    
    player->state = PLAYER_STOPPED;
    player->volume = 100;
    player->running = true;
    
    /* Play startup melody */
    music_play_melody(melody_startup, 4);
    
    return player;
}

/*
 * Destroy music player
 */
void music_player_destroy(music_player_t *player) {
    if (player) {
        speaker_off();
        music_player_clear_playlist(player);
        if (player->window) javier_window_destroy(player->window);
        kfree(player);
    }
}

/*
 * Draw progress bar
 */
static void draw_progress_bar(int x, int y, int width, int height,
                              uint32_t position, uint32_t duration,
                              color_t fg, color_t bg) {
    fb_fill_rect(x, y, width, height, bg);
    
    if (duration > 0) {
        int progress_width = (position * width) / duration;
        fb_fill_rect(x, y, progress_width, height, fg);
    }
    
    /* Border */
    fb_draw_rect(x, y, width, height, RGB(100, 100, 100));
}

/*
 * Draw button
 */
static void draw_button(int x, int y, int size, const char *icon, 
                        bool active, color_t color) {
    fb_fill_rect(x, y, size, size, active ? RGB(80, 80, 80) : RGB(50, 50, 50));
    fb_draw_rect(x, y, size, size, RGB(100, 100, 100));
    
    int cx = x + size / 2;
    int cy = y + size / 2;
    
    if (strcmp(icon, "play") == 0) {
        /* Play triangle */
        for (int i = 0; i < 10; i++) {
            fb_draw_line(cx - 4 + i/2, cy - 5 + i, cx - 4 + i/2, cy + 5 - i, color);
        }
    } else if (strcmp(icon, "pause") == 0) {
        fb_fill_rect(cx - 5, cy - 5, 4, 10, color);
        fb_fill_rect(cx + 1, cy - 5, 4, 10, color);
    } else if (strcmp(icon, "stop") == 0) {
        fb_fill_rect(cx - 5, cy - 5, 10, 10, color);
    } else if (strcmp(icon, "prev") == 0) {
        fb_fill_rect(cx - 5, cy - 5, 2, 10, color);
        for (int i = 0; i < 8; i++) {
            fb_draw_line(cx + 4 - i, cy - 4 + i/2, cx + 4 - i, cy + 4 - i/2, color);
        }
    } else if (strcmp(icon, "next") == 0) {
        fb_fill_rect(cx + 3, cy - 5, 2, 10, color);
        for (int i = 0; i < 8; i++) {
            fb_draw_line(cx - 5 + i, cy - 4 + i/2, cx - 5 + i, cy + 4 - i/2, color);
        }
    }
}

/*
 * Format time
 */
static void format_time(uint32_t ms, char *buf, size_t size) {
    uint32_t secs = ms / 1000;
    uint32_t mins = secs / 60;
    secs %= 60;
    snprintf(buf, size, "%d:%02d", mins, secs);
}

/*
 * Draw music player
 */
void music_player_draw(music_player_t *player) {
    if (!player || !player->window) return;
    
    int cx = player->window->bounds.x + player->window->client.x;
    int cy = player->window->bounds.y + player->window->client.y;
    int cw = player->window->client.width;
    int ch = player->window->client.height;
    
    /* Background gradient */
    for (int y = 0; y < ch; y++) {
        int shade = 25 + (y * 15) / ch;
        fb_draw_line(cx, cy + y, cx + cw - 1, cy + y, RGB(shade, shade, shade + 10));
    }
    
    /* Album art placeholder */
    int art_size = 150;
    int art_x = cx + 20;
    int art_y = cy + 20;
    fb_fill_rect(art_x, art_y, art_size, art_size, RGB(60, 60, 70));
    fb_draw_rect(art_x, art_y, art_size, art_size, RGB(100, 100, 110));
    
    /* Draw music note icon */
    int note_x = art_x + art_size / 2;
    int note_y = art_y + art_size / 2;
    fb_fill_circle(note_x - 15, note_y + 20, 12, RGB(120, 120, 140));
    fb_fill_circle(note_x + 15, note_y + 20, 12, RGB(120, 120, 140));
    fb_fill_rect(note_x - 6, note_y - 30, 4, 50, RGB(120, 120, 140));
    fb_fill_rect(note_x + 24, note_y - 25, 4, 45, RGB(120, 120, 140));
    fb_fill_rect(note_x - 6, note_y - 30, 34, 4, RGB(120, 120, 140));
    
    /* Track info */
    int info_x = art_x + art_size + 20;
    int info_y = art_y;
    
    if (player->current_track) {
        fb_draw_string(info_x, info_y, player->current_track->info.title,
                       RGB(255, 255, 255), 0);
        fb_draw_string(info_x, info_y + 20, player->current_track->info.artist,
                       RGB(180, 180, 180), 0);
        fb_draw_string(info_x, info_y + 40, player->current_track->info.album,
                       RGB(140, 140, 140), 0);
    } else {
        fb_draw_string(info_x, info_y, "No Track Playing",
                       RGB(255, 255, 255), 0);
        fb_draw_string(info_x, info_y + 20, "Add music to your playlist",
                       RGB(140, 140, 140), 0);
    }
    
    /* Progress bar */
    int prog_x = cx + 20;
    int prog_y = cy + ch - 80;
    int prog_w = cw - 40;
    int prog_h = 8;
    
    uint32_t duration = player->current_track ? 
                        player->current_track->info.duration_ms : 0;
    draw_progress_bar(prog_x, prog_y, prog_w, prog_h,
                      player->position_ms, duration,
                      RGB(100, 180, 255), RGB(60, 60, 60));
    
    /* Time display */
    char time_str[32];
    format_time(player->position_ms, time_str, sizeof(time_str));
    fb_draw_string(prog_x, prog_y + 12, time_str, RGB(180, 180, 180), 0);
    
    format_time(duration, time_str, sizeof(time_str));
    fb_draw_string(prog_x + prog_w - 40, prog_y + 12, time_str, RGB(180, 180, 180), 0);
    
    /* Control buttons */
    int btn_y = cy + ch - 50;
    int btn_size = 40;
    int btn_gap = 10;
    int btn_start = cx + cw / 2 - (btn_size * 5 + btn_gap * 4) / 2;
    
    draw_button(btn_start, btn_y, btn_size, "prev", false, RGB(200, 200, 200));
    draw_button(btn_start + btn_size + btn_gap, btn_y, btn_size, "stop",
                player->state == PLAYER_STOPPED, RGB(200, 200, 200));
    draw_button(btn_start + (btn_size + btn_gap) * 2, btn_y, btn_size,
                player->state == PLAYER_PLAYING ? "pause" : "play",
                player->state == PLAYER_PLAYING, RGB(100, 200, 255));
    draw_button(btn_start + (btn_size + btn_gap) * 3, btn_y, btn_size, "next",
                false, RGB(200, 200, 200));
    
    /* Volume */
    int vol_x = cx + cw - 120;
    int vol_y = btn_y + 15;
    fb_draw_string(vol_x, vol_y, "Vol:", RGB(150, 150, 150), 0);
    draw_progress_bar(vol_x + 35, vol_y, 70, 10, player->volume, 100,
                      RGB(100, 180, 255), RGB(60, 60, 60));
    
    /* State indicator */
    const char *state_str = "Stopped";
    color_t state_color = RGB(150, 150, 150);
    if (player->state == PLAYER_PLAYING) {
        state_str = "Playing";
        state_color = RGB(100, 200, 100);
    } else if (player->state == PLAYER_PAUSED) {
        state_str = "Paused";
        state_color = RGB(200, 200, 100);
    }
    fb_draw_string(cx + 20, btn_y + 15, state_str, state_color, 0);
}

/*
 * Update music player
 */
void music_player_update(music_player_t *player) {
    if (!player || !player->running) return;
    
    /* Update position if playing */
    if (player->state == PLAYER_PLAYING) {
        player->position_ms += 100;  /* Approximate */
        
        if (player->current_track && 
            player->position_ms >= player->current_track->info.duration_ms) {
            if (player->repeat) {
                player->position_ms = 0;
            } else {
                music_player_next(player);
            }
        }
    }
    
    /* Handle input */
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_UP:
                    if (player->volume < 100) player->volume += 10;
                    break;
                case KEY_DOWN:
                    if (player->volume > 0) player->volume -= 10;
                    break;
                case KEY_RIGHT:
                    music_player_next(player);
                    break;
                case KEY_LEFT:
                    music_player_prev(player);
                    break;
                case KEY_ESC:
                    player->running = false;
                    break;
                default:
                    /* Handle ASCII keys */
                    if (event.ascii == ' ') {
                        if (player->state == PLAYER_PLAYING)
                            music_player_pause(player);
                        else
                            music_player_play(player);
                    } else if (event.ascii == 's' || event.ascii == 'S') {
                        music_player_stop(player);
                    } else if (event.ascii == 'n' || event.ascii == 'N') {
                        music_player_next(player);
                    } else if (event.ascii == 'p' || event.ascii == 'P') {
                        music_player_prev(player);
                    } else if (event.ascii == 'r' || event.ascii == 'R') {
                        player->repeat = !player->repeat;
                    }
                    break;
            }
        }
    }
}

/*
 * Play
 */
void music_player_play(music_player_t *player) {
    if (!player) return;
    
    if (!player->current_track && player->playlist) {
        player->current_track = player->playlist;
    }
    
    player->state = PLAYER_PLAYING;
    
    /* Play a note to indicate playback started */
    speaker_beep(NOTE_C5, 50);
}

/*
 * Pause
 */
void music_player_pause(music_player_t *player) {
    if (!player) return;
    player->state = PLAYER_PAUSED;
    speaker_off();
}

/*
 * Stop
 */
void music_player_stop(music_player_t *player) {
    if (!player) return;
    player->state = PLAYER_STOPPED;
    player->position_ms = 0;
    speaker_off();
}

/*
 * Next track
 */
void music_player_next(music_player_t *player) {
    if (!player || !player->current_track) return;
    
    if (player->current_track->next) {
        player->current_track = player->current_track->next;
    } else {
        player->current_track = player->playlist;  /* Loop to start */
    }
    player->position_ms = 0;
}

/*
 * Previous track
 */
void music_player_prev(music_player_t *player) {
    if (!player || !player->current_track) return;
    
    if (player->position_ms > 3000) {
        /* Restart current track if more than 3 seconds in */
        player->position_ms = 0;
    } else if (player->current_track->prev) {
        player->current_track = player->current_track->prev;
        player->position_ms = 0;
    }
}

/*
 * Add track to playlist
 */
void music_player_add_track(music_player_t *player, const char *path) {
    if (!player || !path) return;
    
    playlist_entry_t *entry = (playlist_entry_t *)kcalloc(1, sizeof(playlist_entry_t));
    if (!entry) return;
    
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    
    /* Extract filename as title */
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    strncpy(entry->info.title, filename, sizeof(entry->info.title) - 1);
    strcpy(entry->info.artist, "Unknown Artist");
    strcpy(entry->info.album, "Unknown Album");
    entry->info.duration_ms = 180000;  /* Default 3 minutes */
    
    /* Add to end of playlist */
    if (!player->playlist) {
        player->playlist = entry;
    } else {
        playlist_entry_t *last = player->playlist;
        while (last->next) last = last->next;
        last->next = entry;
        entry->prev = last;
    }
}

/*
 * Clear playlist
 */
void music_player_clear_playlist(music_player_t *player) {
    if (!player) return;
    
    playlist_entry_t *entry = player->playlist;
    while (entry) {
        playlist_entry_t *next = entry->next;
        kfree(entry);
        entry = next;
    }
    
    player->playlist = NULL;
    player->current_track = NULL;
}
