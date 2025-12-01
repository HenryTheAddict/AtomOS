/*
 * AtomOS Beat Maker Implementation
 * Drum machine with step sequencer
 */

#include "beatmaker.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"
#include "../../apps/music/music.h"

/* Color component extraction macros */
#define BM_COLOR_R(c) (((c) >> 16) & 0xFF)
#define BM_COLOR_G(c) (((c) >> 8) & 0xFF)
#define BM_COLOR_B(c) ((c) & 0xFF)

/* Color scheme */
#define BM_BG           RGB(25, 25, 30)
#define BM_PANEL        RGB(40, 40, 45)
#define BM_GRID         RGB(55, 55, 60)
#define BM_TEXT         RGB(220, 220, 220)
#define BM_ACCENT       RGB(255, 120, 50)
#define BM_STEP_OFF     RGB(60, 60, 65)
#define BM_STEP_ON      RGB(255, 100, 50)
#define BM_PLAYHEAD     RGB(100, 255, 100)

/* Drum names */
static const char *drum_names[] = {
    "KICK", "SNARE", "HH-C", "HH-O", "TOM-H", "TOM-M", "TOM-L", "CLAP",
    "CRASH", "RIDE", "COWBL", "SHAKR"
};

/* Drum frequencies (for PC speaker) */
static const int drum_freqs[] = {
    60,    /* Kick - low */
    200,   /* Snare */
    800,   /* Hi-hat closed */
    600,   /* Hi-hat open */
    300,   /* Tom high */
    200,   /* Tom mid */
    150,   /* Tom low */
    400,   /* Clap */
    500,   /* Crash */
    700,   /* Ride */
    1000,  /* Cowbell */
    900    /* Shaker */
};

/* Drum durations */
static const int drum_durations[] = {
    50,   /* Kick */
    30,   /* Snare */
    10,   /* Hi-hat closed */
    40,   /* Hi-hat open */
    40,   /* Tom high */
    45,   /* Tom mid */
    50,   /* Tom low */
    25,   /* Clap */
    100,  /* Crash */
    60,   /* Ride */
    20,   /* Cowbell */
    15    /* Shaker */
};

/* Track colors */
static color_t track_colors[] = {
    RGB(255, 80, 80),    /* Kick - red */
    RGB(255, 180, 80),   /* Snare - orange */
    RGB(255, 255, 80),   /* HH-C - yellow */
    RGB(200, 255, 80),   /* HH-O - lime */
    RGB(80, 255, 180),   /* Tom H - cyan */
    RGB(80, 180, 255),   /* Tom M - sky */
    RGB(80, 80, 255),    /* Tom L - blue */
    RGB(180, 80, 255)    /* Clap - purple */
};

/* Create beat maker */
beatmaker_t *beatmaker_create(void) {
    beatmaker_t *bm = (beatmaker_t *)kcalloc(1, sizeof(beatmaker_t));
    if (!bm) return NULL;
    
    bm->window = javier_window_create("Beat Maker", 50, 50, 650, 450, WIN_DEFAULT);
    bm->bpm = 120;
    bm->running = true;
    
    /* Set default drum sounds */
    bm->drum_sounds[0] = DRUM_KICK;
    bm->drum_sounds[1] = DRUM_SNARE;
    bm->drum_sounds[2] = DRUM_HIHAT_CLOSED;
    bm->drum_sounds[3] = DRUM_HIHAT_OPEN;
    bm->drum_sounds[4] = DRUM_TOM_HIGH;
    bm->drum_sounds[5] = DRUM_TOM_MID;
    bm->drum_sounds[6] = DRUM_TOM_LOW;
    bm->drum_sounds[7] = DRUM_CLAP;
    
    /* Set default volumes */
    for (int i = 0; i < BM_TRACKS; i++) {
        bm->track_volumes[i] = 100;
    }
    
    /* Initialize first pattern */
    strcpy(bm->patterns[0].name, "Pattern 1");
    
    /* Load default beat */
    beatmaker_load_preset(bm, 0);
    
    return bm;
}

/* Destroy beat maker */
void beatmaker_destroy(beatmaker_t *bm) {
    if (bm) {
        if (bm->window) {
            javier_window_destroy(bm->window);
        }
        kfree(bm);
    }
}

/* Trigger a drum sound */
void beatmaker_trigger_sound(beatmaker_t *bm, drum_type_t drum, int velocity) {
    (void)bm;
    (void)velocity;
    
    if (drum >= DRUM_COUNT) return;
    
    int freq = drum_freqs[drum];
    int duration = drum_durations[drum];
    
    /* Use PC speaker */
    speaker_beep(freq, duration);
}

/* Load preset pattern */
void beatmaker_load_preset(beatmaker_t *bm, int preset) {
    beat_pattern_t *p = &bm->patterns[bm->current_pattern];
    
    /* Clear pattern */
    memset(p->steps, 0, sizeof(p->steps));
    for (int t = 0; t < BM_TRACKS; t++) {
        for (int s = 0; s < BM_STEPS; s++) {
            p->velocities[t][s] = 100;
        }
    }
    
    switch (preset) {
        case 0:  /* Basic 4/4 */
            /* Kick on 1, 5, 9, 13 */
            p->steps[0][0] = p->steps[0][4] = p->steps[0][8] = p->steps[0][12] = true;
            /* Snare on 5, 13 */
            p->steps[1][4] = p->steps[1][12] = true;
            /* Hi-hat on every step */
            for (int i = 0; i < BM_STEPS; i++) {
                p->steps[2][i] = true;
            }
            break;
            
        case 1:  /* Hip Hop */
            p->steps[0][0] = p->steps[0][6] = p->steps[0][10] = true;
            p->steps[1][4] = p->steps[1][12] = true;
            p->steps[2][0] = p->steps[2][2] = p->steps[2][4] = p->steps[2][6] = true;
            p->steps[2][8] = p->steps[2][10] = p->steps[2][12] = p->steps[2][14] = true;
            p->steps[3][6] = p->steps[3][14] = true;
            break;
            
        case 2:  /* House */
            /* Four on the floor kick */
            p->steps[0][0] = p->steps[0][4] = p->steps[0][8] = p->steps[0][12] = true;
            /* Clap/snare on 2, 4 */
            p->steps[7][4] = p->steps[7][12] = true;
            /* Offbeat hi-hat */
            p->steps[3][2] = p->steps[3][6] = p->steps[3][10] = p->steps[3][14] = true;
            break;
            
        case 3:  /* Drum & Bass */
            p->steps[0][0] = p->steps[0][10] = true;
            p->steps[1][4] = p->steps[1][12] = true;
            for (int i = 0; i < BM_STEPS; i += 2) {
                p->steps[2][i] = true;
            }
            break;
            
        case 4:  /* Reggae */
            p->steps[0][6] = p->steps[0][14] = true;
            p->steps[1][6] = p->steps[1][14] = true;
            p->steps[2][0] = p->steps[2][4] = p->steps[2][8] = p->steps[2][12] = true;
            break;
            
        default:
            break;
    }
}

/* Toggle step */
void beatmaker_toggle_step(beatmaker_t *bm, int track, int step) {
    if (track < 0 || track >= BM_TRACKS || step < 0 || step >= BM_STEPS) return;
    
    beat_pattern_t *p = &bm->patterns[bm->current_pattern];
    p->steps[track][step] = !p->steps[track][step];
    
    /* Play sound when enabled */
    if (p->steps[track][step]) {
        beatmaker_trigger_sound(bm, bm->drum_sounds[track], p->velocities[track][step]);
    }
}

/* Clear pattern */
void beatmaker_clear_pattern(beatmaker_t *bm) {
    beat_pattern_t *p = &bm->patterns[bm->current_pattern];
    memset(p->steps, 0, sizeof(p->steps));
}

/* Copy pattern */
void beatmaker_copy_pattern(beatmaker_t *bm, int from, int to) {
    if (from < 0 || from >= BM_PATTERNS || to < 0 || to >= BM_PATTERNS) return;
    memcpy(&bm->patterns[to], &bm->patterns[from], sizeof(beat_pattern_t));
}

/* Play */
void beatmaker_play(beatmaker_t *bm) {
    bm->playing = true;
    bm->last_step_time = timer_get_ticks();
}

/* Stop */
void beatmaker_stop(beatmaker_t *bm) {
    bm->playing = false;
    bm->current_step = 0;
}

/* Draw beat maker */
void beatmaker_draw(beatmaker_t *bm) {
    if (!bm || !bm->window) return;
    
    int x = bm->window->bounds.x + bm->window->client.x;
    int y = bm->window->bounds.y + bm->window->client.y;
    int w = bm->window->client.width;
    int h = bm->window->client.height;
    
    /* Background */
    fb_fill_rect(x, y, w, h, BM_BG);
    
    /* Header */
    fb_fill_rect(x, y, w, 50, BM_PANEL);
    
    /* Title */
    fb_draw_string(x + 10, y + 8, "BEAT MAKER", RGB(255, 150, 50), 0);
    
    /* BPM */
    char buf[32];
    snprintf(buf, sizeof(buf), "BPM: %d", bm->bpm);
    fb_draw_string(x + 150, y + 8, buf, BM_TEXT, 0);
    
    /* Transport */
    int tx = x + 280;
    fb_fill_rect(tx, y + 8, 30, 30, bm->playing ? RGB(100, 255, 100) : BM_GRID);
    fb_draw_string(tx + 10, y + 15, ">", BM_TEXT, 0);
    
    fb_fill_rect(tx + 35, y + 8, 30, 30, BM_GRID);
    fb_fill_rect(tx + 43, y + 16, 14, 14, BM_TEXT);
    
    /* Pattern selector */
    snprintf(buf, sizeof(buf), "Pattern: %d", bm->current_pattern + 1);
    fb_draw_string(x + 380, y + 8, buf, BM_TEXT, 0);
    
    /* Preset buttons */
    fb_draw_string(x + 10, y + 32, "Presets:", RGB(150, 150, 150), 0);
    const char *presets[] = {"4/4", "HIP", "HSE", "DNB", "REG"};
    for (int i = 0; i < 5; i++) {
        fb_fill_rect(x + 80 + i * 40, y + 28, 35, 18, BM_GRID);
        fb_draw_string(x + 85 + i * 40, y + 32, presets[i], BM_TEXT, 0);
    }
    
    /* Grid area */
    int grid_y = y + 60;
    int track_h = 35;
    int step_w = 32;
    int label_w = 70;
    
    /* Track labels and grid */
    for (int t = 0; t < BM_TRACKS; t++) {
        int ty = grid_y + t * track_h;
        
        /* Track label background */
        fb_fill_rect(x, ty, label_w, track_h - 2, 
                    (t == bm->cursor_y) ? BM_PANEL : RGB(35, 35, 40));
        
        /* Track color bar */
        fb_fill_rect(x, ty, 4, track_h - 2, track_colors[t]);
        
        /* Track name */
        fb_draw_string(x + 8, ty + 10, drum_names[bm->drum_sounds[t]], BM_TEXT, 0);
        
        /* Mute/Solo indicators */
        if (bm->track_muted[t]) {
            fb_draw_string(x + 55, ty + 10, "M", RGB(255, 100, 100), 0);
        }
        if (bm->track_solo[t]) {
            fb_draw_string(x + 55, ty + 20, "S", RGB(255, 200, 50), 0);
        }
        
        /* Steps */
        beat_pattern_t *p = &bm->patterns[bm->current_pattern];
        for (int s = 0; s < BM_STEPS; s++) {
            int sx = x + label_w + s * step_w;
            
            /* Step background - darker every 4 steps */
            color_t bg = (s % 4 == 0) ? RGB(45, 45, 50) : BM_STEP_OFF;
            
            /* Highlight current cursor position */
            if (s == bm->cursor_x && t == bm->cursor_y) {
                bg = RGB(80, 80, 90);
            }
            
            fb_fill_rect(sx + 1, ty + 1, step_w - 2, track_h - 4, bg);
            
            /* Draw step if active */
            if (p->steps[t][s]) {
                color_t step_color = track_colors[t];
                
                /* Velocity affects brightness */
                int vel = p->velocities[t][s];
                step_color = RGB(
                    (BM_COLOR_R(step_color) * vel) / 127,
                    (BM_COLOR_G(step_color) * vel) / 127,
                    (BM_COLOR_B(step_color) * vel) / 127
                );
                
                fb_fill_rect(sx + 3, ty + 3, step_w - 6, track_h - 8, step_color);
            }
        }
    }
    
    /* Playhead */
    if (bm->playing) {
        int ph_x = x + label_w + bm->current_step * step_w + step_w / 2;
        fb_draw_line(ph_x, grid_y, ph_x, grid_y + BM_TRACKS * track_h, BM_PLAYHEAD);
    }
    
    /* Step numbers */
    for (int s = 0; s < BM_STEPS; s++) {
        int sx = x + label_w + s * step_w;
        snprintf(buf, sizeof(buf), "%d", s + 1);
        fb_draw_string(sx + 10, grid_y + BM_TRACKS * track_h + 5, buf, 
                      RGB(100, 100, 100), 0);
    }
    
    /* Bottom panel - keyboard hints */
    int bottom_y = y + h - 30;
    fb_fill_rect(x, bottom_y, w, 30, BM_PANEL);
    fb_draw_string(x + 10, bottom_y + 8, 
                  "SPACE: Play/Stop  ARROWS: Navigate  ENTER: Toggle  C: Clear  1-5: Presets  M: Mute",
                  RGB(150, 150, 150), 0);
}

/* Update beat maker */
void beatmaker_update(beatmaker_t *bm) {
    if (!bm) return;
    
    /* Handle input */
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    bm->running = false;
                    break;
                    
                case KEY_UP:
                    if (bm->cursor_y > 0) bm->cursor_y--;
                    break;
                case KEY_DOWN:
                    if (bm->cursor_y < BM_TRACKS - 1) bm->cursor_y++;
                    break;
                case KEY_LEFT:
                    if (bm->cursor_x > 0) bm->cursor_x--;
                    break;
                case KEY_RIGHT:
                    if (bm->cursor_x < BM_STEPS - 1) bm->cursor_x++;
                    break;
                    
                case KEY_ENTER:
                    beatmaker_toggle_step(bm, bm->cursor_y, bm->cursor_x);
                    break;
                    
                default:
                    if (event.ascii == ' ') {
                        if (bm->playing) beatmaker_stop(bm);
                        else beatmaker_play(bm);
                    } else if (event.ascii == 'c' || event.ascii == 'C') {
                        beatmaker_clear_pattern(bm);
                    } else if (event.ascii == 'm' || event.ascii == 'M') {
                        bm->track_muted[bm->cursor_y] = !bm->track_muted[bm->cursor_y];
                    } else if (event.ascii == 's' || event.ascii == 'S') {
                        bm->track_solo[bm->cursor_y] = !bm->track_solo[bm->cursor_y];
                    } else if (event.ascii >= '1' && event.ascii <= '5') {
                        beatmaker_load_preset(bm, event.ascii - '1');
                    } else if (event.ascii == '+' || event.ascii == '=') {
                        if (bm->bpm < 300) bm->bpm += 5;
                    } else if (event.ascii == '-' || event.ascii == '_') {
                        if (bm->bpm > 40) bm->bpm -= 5;
                    }
                    break;
            }
        }
    }
    
    /* Update playback */
    if (bm->playing) {
        uint64_t now = timer_get_ticks();
        
        /* Calculate step duration based on BPM */
        /* 16 steps per bar, so each step = 60000 / (bpm * 4) ms */
        /* Timer ticks at ~18.2 Hz, so ticks = ms / 55 */
        uint64_t step_ticks = (60 * 18) / (bm->bpm * 4);
        if (step_ticks < 1) step_ticks = 1;
        
        if (now - bm->last_step_time >= step_ticks) {
            /* Trigger sounds for this step */
            beat_pattern_t *p = &bm->patterns[bm->current_pattern];
            
            /* Check for any solo'd tracks */
            bool has_solo = false;
            for (int t = 0; t < BM_TRACKS; t++) {
                if (bm->track_solo[t]) {
                    has_solo = true;
                    break;
                }
            }
            
            for (int t = 0; t < BM_TRACKS; t++) {
                if (bm->track_muted[t]) continue;
                if (has_solo && !bm->track_solo[t]) continue;
                
                if (p->steps[t][bm->current_step]) {
                    beatmaker_trigger_sound(bm, bm->drum_sounds[t], 
                                           p->velocities[t][bm->current_step]);
                }
            }
            
            /* Advance step */
            bm->current_step = (bm->current_step + 1) % BM_STEPS;
            bm->last_step_time = now;
        }
    }
    
    beatmaker_draw(bm);
}
