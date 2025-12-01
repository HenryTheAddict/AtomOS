/*
 * AtomOS DAW Implementation
 * Digital Audio Workstation with synthesizers and effects
 */

#include "daw.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/timer.h"

/* Color scheme */
#define DAW_BG          RGB(30, 30, 35)
#define DAW_PANEL       RGB(45, 45, 50)
#define DAW_BORDER      RGB(60, 60, 65)
#define DAW_TEXT        RGB(220, 220, 220)
#define DAW_ACCENT      RGB(100, 150, 255)
#define DAW_GRID        RGB(50, 50, 55)
#define DAW_PLAYHEAD    RGB(255, 100, 100)

/* Track colors */
static color_t track_colors[] = {
    RGB(255, 100, 100), RGB(100, 255, 100), RGB(100, 100, 255),
    RGB(255, 255, 100), RGB(255, 100, 255), RGB(100, 255, 255),
    RGB(255, 150, 50),  RGB(150, 255, 50),  RGB(50, 150, 255),
    RGB(255, 50, 150),  RGB(50, 255, 150),  RGB(150, 50, 255),
    RGB(200, 200, 200), RGB(150, 150, 150), RGB(100, 100, 100),
    RGB(200, 150, 100)
};

/* Note names - used for piano roll display */
__attribute__((unused))
static const char *note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

/* Instrument names - used for track display */
__attribute__((unused))
static const char *inst_names[] = {
    "None", "Sine", "Square", "Saw", "Triangle", "Noise",
    "Piano", "Bass", "Pad", "Lead", "Strings", "Brass", "Drums", "Sampler"
};

/* Simple sine approximation */
static int daw_sin(int x) {
    /* x is in units of 1024 = 2*PI */
    x = x & 1023;
    if (x < 256) return x;
    if (x < 512) return 512 - x;
    if (x < 768) return -(x - 512);
    return -(1024 - x);
}

/* Create DAW */
daw_state_t *daw_create(void) {
    daw_state_t *daw = (daw_state_t *)kcalloc(1, sizeof(daw_state_t));
    if (!daw) return NULL;
    
    daw->window = javier_window_create("AtomOS DAW", 20, 20, 900, 600, WIN_DEFAULT);
    daw->view = VIEW_ARRANGE;
    daw->zoom_x = 20;
    daw->zoom_y = 30;
    daw->octave = 4;
    daw->running = true;
    
    daw_new_project(daw);
    
    return daw;
}

/* Destroy DAW */
void daw_destroy(daw_state_t *daw) {
    if (daw) {
        if (daw->window) {
            javier_window_destroy(daw->window);
        }
        kfree(daw);
    }
}

/* Create new project */
void daw_new_project(daw_state_t *daw) {
    memset(&daw->project, 0, sizeof(daw_project_t));
    
    strcpy(daw->project.name, "Untitled Project");
    daw->project.bpm = 120;
    daw->project.time_sig_num = 4;
    daw->project.time_sig_den = 4;
    daw->project.song_length = 16;
    daw->project.loop_enabled = true;
    daw->project.loop_start = 0;
    daw->project.loop_end = 4;
    
    /* Create default tracks */
    daw_add_track(daw, "Drums", INST_DRUMS);
    daw_add_track(daw, "Bass", INST_BASS);
    daw_add_track(daw, "Lead", INST_LEAD);
    daw_add_track(daw, "Pad", INST_PAD);
    
    /* Create default pattern */
    daw_pattern_t *pattern = daw_create_pattern(daw, "Pattern 1");
    if (pattern) {
        /* Add some demo notes */
        daw_add_note(pattern, 60, 100, 0, 4);   /* C4 */
        daw_add_note(pattern, 64, 100, 4, 4);   /* E4 */
        daw_add_note(pattern, 67, 100, 8, 4);   /* G4 */
        daw_add_note(pattern, 72, 100, 12, 4);  /* C5 */
    }
}

/* Add track */
void daw_add_track(daw_state_t *daw, const char *name, daw_inst_type_t inst) {
    if (daw->project.track_count >= DAW_MAX_TRACKS) return;
    
    daw_track_t *track = &daw->project.tracks[daw->project.track_count];
    memset(track, 0, sizeof(daw_track_t));
    
    strncpy(track->name, name, sizeof(track->name) - 1);
    track->volume = 100;
    track->pan = 0;
    track->instrument.type = inst;
    track->instrument.volume = 100;
    track->instrument.attack = 10;
    track->instrument.decay = 20;
    track->instrument.sustain = 70;
    track->instrument.release = 30;
    track->color = track_colors[daw->project.track_count % 16];
    
    for (int i = 0; i < DAW_MAX_PATTERNS; i++) {
        track->pattern_indices[i] = -1;
    }
    
    daw->project.track_count++;
}

/* Remove track */
void daw_remove_track(daw_state_t *daw, int index) {
    if (index < 0 || index >= daw->project.track_count) return;
    
    for (int i = index; i < daw->project.track_count - 1; i++) {
        daw->project.tracks[i] = daw->project.tracks[i + 1];
    }
    daw->project.track_count--;
}

/* Mute track */
void daw_mute_track(daw_state_t *daw, int index, bool mute) {
    if (index < 0 || index >= daw->project.track_count) return;
    daw->project.tracks[index].mute = mute;
}

/* Solo track */
void daw_solo_track(daw_state_t *daw, int index, bool solo) {
    if (index < 0 || index >= daw->project.track_count) return;
    daw->project.tracks[index].solo = solo;
}

/* Create pattern */
daw_pattern_t *daw_create_pattern(daw_state_t *daw, const char *name) {
    if (daw->project.pattern_count >= DAW_MAX_PATTERNS) return NULL;
    
    daw_pattern_t *pattern = &daw->project.patterns[daw->project.pattern_count];
    memset(pattern, 0, sizeof(daw_pattern_t));
    
    strncpy(pattern->name, name, sizeof(pattern->name) - 1);
    pattern->length = DAW_PATTERN_LEN;
    
    daw->project.pattern_count++;
    return pattern;
}

/* Add note to pattern */
void daw_add_note(daw_pattern_t *pattern, int note, int velocity, int start, int duration) {
    if (!pattern || pattern->note_count >= DAW_MAX_NOTES) return;
    
    daw_note_t *n = &pattern->notes[pattern->note_count];
    n->note = note;
    n->velocity = velocity;
    n->start = start;
    n->duration = duration;
    n->active = true;
    
    pattern->note_count++;
}

/* Remove note from pattern */
void daw_remove_note(daw_pattern_t *pattern, int index) {
    if (!pattern || index < 0 || index >= pattern->note_count) return;
    
    for (int i = index; i < pattern->note_count - 1; i++) {
        pattern->notes[i] = pattern->notes[i + 1];
    }
    pattern->note_count--;
}

/* Set track volume */
void daw_set_track_volume(daw_state_t *daw, int track, int volume) {
    if (track < 0 || track >= daw->project.track_count) return;
    daw->project.tracks[track].volume = volume < 0 ? 0 : (volume > 127 ? 127 : volume);
}

/* Set track pan */
void daw_set_track_pan(daw_state_t *daw, int track, int pan) {
    if (track < 0 || track >= daw->project.track_count) return;
    daw->project.tracks[track].pan = pan < -64 ? -64 : (pan > 63 ? 63 : pan);
}

/* Add effect to track */
void daw_add_effect(daw_state_t *daw, int track, daw_fx_type_t type) {
    if (track < 0 || track >= daw->project.track_count) return;
    
    daw_track_t *t = &daw->project.tracks[track];
    for (int i = 0; i < DAW_MAX_EFFECTS; i++) {
        if (t->effects[i].type == FX_NONE) {
            t->effects[i].type = type;
            t->effects[i].enabled = true;
            break;
        }
    }
}

/* Synthesize sample */
int16_t daw_synth_sample(daw_instrument_t *inst, int note, int time, int env_time) {
    if (!inst || inst->type == INST_NONE) return 0;
    
    /* Calculate frequency from MIDI note */
    /* f = 440 * 2^((note-69)/12) */
    int freq = 440 * (1 << ((note - 69 + 36) / 12));  /* Simplified */
    freq = freq >> ((69 - note + 36) / 12);
    
    int sample = 0;
    int phase = (time * freq) % 44100;
    
    switch (inst->type) {
        case INST_SINE:
        case INST_PIANO:
        case INST_PAD:
            sample = daw_sin(phase * 1024 / 44100) * 256;
            break;
            
        case INST_SQUARE:
        case INST_LEAD:
            sample = (phase < 22050) ? 32767 : -32768;
            break;
            
        case INST_SAW:
        case INST_BASS:
            sample = (phase * 65536 / 44100) - 32768;
            break;
            
        case INST_TRIANGLE:
        case INST_STRINGS:
            if (phase < 11025) sample = phase * 32767 / 11025;
            else if (phase < 33075) sample = 32767 - (phase - 11025) * 65536 / 22050;
            else sample = -32768 + (phase - 33075) * 32767 / 11025;
            break;
            
        case INST_NOISE:
        case INST_DRUMS:
            sample = (int)((time * 1103515245 + 12345) >> 16) - 32768;
            break;
            
        default:
            break;
    }
    
    /* Apply envelope */
    int env = 127;
    if (env_time < inst->attack) {
        env = env_time * 127 / (inst->attack + 1);
    } else if (env_time < inst->attack + inst->decay) {
        int decay_pos = env_time - inst->attack;
        env = 127 - (127 - inst->sustain) * decay_pos / (inst->decay + 1);
    } else {
        env = inst->sustain;
    }
    
    /* Apply volume */
    sample = (sample * inst->volume * env) / (127 * 127);
    
    return (int16_t)sample;
}

/* Draw toolbar */
static void daw_draw_toolbar(daw_state_t *daw) {
    int x = daw->window->bounds.x + daw->window->client.x;
    int y = daw->window->bounds.y + daw->window->client.y;
    int w = daw->window->client.width;
    
    /* Toolbar background */
    fb_fill_rect(x, y, w, 40, DAW_PANEL);
    fb_draw_line(x, y + 40, x + w, y + 40, DAW_BORDER);
    
    /* Transport controls */
    int bx = x + 10;
    
    /* Play button */
    fb_fill_rect(bx, y + 8, 24, 24, daw->playing ? RGB(100, 200, 100) : DAW_GRID);
    fb_draw_string(bx + 6, y + 12, ">", DAW_TEXT, 0);
    
    /* Stop button */
    fb_fill_rect(bx + 30, y + 8, 24, 24, DAW_GRID);
    fb_fill_rect(bx + 36, y + 14, 12, 12, DAW_TEXT);
    
    /* Record button */
    fb_fill_rect(bx + 60, y + 8, 24, 24, daw->recording ? RGB(255, 100, 100) : DAW_GRID);
    fb_fill_rect(bx + 68, y + 16, 8, 8, RGB(255, 50, 50));
    
    /* BPM */
    char buf[32];
    snprintf(buf, sizeof(buf), "BPM: %d", daw->project.bpm);
    fb_draw_string(x + 120, y + 14, buf, DAW_TEXT, 0);
    
    /* Time signature */
    snprintf(buf, sizeof(buf), "%d/%d", daw->project.time_sig_num, daw->project.time_sig_den);
    fb_draw_string(x + 200, y + 14, buf, DAW_TEXT, 0);
    
    /* Position */
    snprintf(buf, sizeof(buf), "%d:%d:%d", daw->current_bar + 1, daw->current_beat + 1, daw->current_tick);
    fb_draw_string(x + 260, y + 14, buf, DAW_TEXT, 0);
    
    /* View tabs */
    const char *tabs[] = {"Arrange", "Piano Roll", "Mixer", "Browser"};
    int tx = x + 400;
    for (int i = 0; i < 4; i++) {
        color_t bg = ((int)daw->view == i) ? DAW_ACCENT : DAW_GRID;
        fb_fill_rect(tx, y + 8, 70, 24, bg);
        fb_draw_string(tx + 5, y + 14, tabs[i], DAW_TEXT, 0);
        tx += 75;
    }
}

/* Draw arrangement view */
static void daw_draw_arrange(daw_state_t *daw) {
    int x = daw->window->bounds.x + daw->window->client.x;
    int y = daw->window->bounds.y + daw->window->client.y + 45;
    int w = daw->window->client.width;
    int h = daw->window->client.height - 45;
    
    /* Track header width */
    int hdr_w = 120;
    
    /* Background */
    fb_fill_rect(x, y, w, h, DAW_BG);
    
    /* Draw track headers */
    for (int t = 0; t < daw->project.track_count; t++) {
        daw_track_t *track = &daw->project.tracks[t];
        int ty = y + t * daw->zoom_y;
        
        if (ty > y + h) break;
        
        color_t bg = (t == daw->selected_track) ? DAW_PANEL : RGB(35, 35, 40);
        fb_fill_rect(x, ty, hdr_w, daw->zoom_y - 1, bg);
        
        /* Track color bar */
        fb_fill_rect(x, ty, 4, daw->zoom_y - 1, track->color);
        
        /* Track name */
        fb_draw_string(x + 8, ty + 5, track->name, DAW_TEXT, 0);
        
        /* Mute/Solo buttons */
        fb_fill_rect(x + 80, ty + 4, 16, 14, track->mute ? RGB(255, 100, 100) : DAW_GRID);
        fb_draw_string(x + 83, ty + 5, "M", DAW_TEXT, 0);
        
        fb_fill_rect(x + 98, ty + 4, 16, 14, track->solo ? RGB(255, 200, 50) : DAW_GRID);
        fb_draw_string(x + 101, ty + 5, "S", DAW_TEXT, 0);
    }
    
    /* Draw timeline grid */
    int timeline_x = x + hdr_w;
    
    /* Bars */
    for (int bar = 0; bar < daw->project.song_length; bar++) {
        int bx = timeline_x + bar * daw->zoom_x * 4 - daw->scroll_x;
        if (bx < timeline_x || bx > x + w) continue;
        
        /* Bar line */
        fb_draw_line(bx, y, bx, y + h, DAW_GRID);
        
        /* Bar number */
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", bar + 1);
        fb_draw_string(bx + 2, y + 2, buf, RGB(100, 100, 100), 0);
    }
    
    /* Draw track patterns */
    for (int t = 0; t < daw->project.track_count; t++) {
        daw_track_t *track = &daw->project.tracks[t];
        int ty = y + t * daw->zoom_y;
        
        if (ty > y + h) break;
        
        for (int bar = 0; bar < daw->project.song_length; bar++) {
            int pat_idx = track->pattern_indices[bar];
            if (pat_idx < 0) continue;
            
            int bx = timeline_x + bar * daw->zoom_x * 4 - daw->scroll_x;
            if (bx < timeline_x - daw->zoom_x * 4 || bx > x + w) continue;
            
            /* Draw pattern block */
            color_t c = track->color;
            fb_fill_rect(bx + 1, ty + 1, daw->zoom_x * 4 - 2, daw->zoom_y - 3, c);
            
            /* Pattern name */
            if (pat_idx < daw->project.pattern_count) {
                fb_draw_string(bx + 4, ty + 4, daw->project.patterns[pat_idx].name, 
                              RGB(255, 255, 255), 0);
            }
        }
    }
    
    /* Draw playhead */
    int playhead_x = timeline_x + daw->current_pos * daw->zoom_x / 4 - daw->scroll_x;
    if (playhead_x >= timeline_x && playhead_x <= x + w) {
        fb_draw_line(playhead_x, y, playhead_x, y + h, DAW_PLAYHEAD);
    }
    
    /* Draw loop region */
    if (daw->project.loop_enabled) {
        int loop_x1 = timeline_x + daw->project.loop_start * daw->zoom_x * 4 - daw->scroll_x;
        int loop_x2 = timeline_x + daw->project.loop_end * daw->zoom_x * 4 - daw->scroll_x;
        fb_fill_rect(loop_x1, y, loop_x2 - loop_x1, 8, RGBA(100, 150, 255, 100));
    }
}

/* Draw piano roll */
static void daw_draw_piano_roll(daw_state_t *daw) {
    int x = daw->window->bounds.x + daw->window->client.x;
    int y = daw->window->bounds.y + daw->window->client.y + 45;
    int w = daw->window->client.width;
    int h = daw->window->client.height - 45;
    
    /* Piano key width */
    int key_w = 40;
    int note_h = 12;
    
    /* Background */
    fb_fill_rect(x, y, w, h, DAW_BG);
    
    /* Draw piano keys */
    for (int n = 127; n >= 0; n--) {
        int ny = y + (127 - n) * note_h - daw->scroll_y;
        if (ny < y - note_h || ny > y + h) continue;
        
        int octave = n / 12;
        int note = n % 12;
        bool is_black = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
        
        color_t key_color = is_black ? RGB(40, 40, 45) : RGB(250, 250, 250);
        fb_fill_rect(x, ny, key_w, note_h - 1, key_color);
        
        /* Note name */
        if (note == 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "C%d", octave);
            fb_draw_string(x + 2, ny + 1, buf, is_black ? DAW_TEXT : RGB(0, 0, 0), 0);
        }
    }
    
    /* Draw note grid */
    int grid_x = x + key_w;
    int grid_w = w - key_w;
    
    for (int n = 127; n >= 0; n--) {
        int ny = y + (127 - n) * note_h - daw->scroll_y;
        if (ny < y - note_h || ny > y + h) continue;
        
        int note = n % 12;
        bool is_black = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
        color_t bg = is_black ? RGB(35, 35, 40) : RGB(40, 40, 45);
        
        fb_fill_rect(grid_x, ny, grid_w, note_h - 1, bg);
    }
    
    /* Draw beat lines */
    for (int beat = 0; beat < DAW_PATTERN_LEN; beat++) {
        int bx = grid_x + beat * daw->zoom_x - daw->scroll_x;
        if (bx < grid_x || bx > x + w) continue;
        
        color_t line_color = (beat % 4 == 0) ? DAW_BORDER : DAW_GRID;
        fb_draw_line(bx, y, bx, y + h, line_color);
    }
    
    /* Draw notes in selected pattern */
    if (daw->selected_pattern >= 0 && daw->selected_pattern < daw->project.pattern_count) {
        daw_pattern_t *pattern = &daw->project.patterns[daw->selected_pattern];
        
        for (int i = 0; i < pattern->note_count; i++) {
            daw_note_t *note = &pattern->notes[i];
            if (!note->active) continue;
            
            int ny = y + (127 - note->note) * note_h - daw->scroll_y;
            int nx = grid_x + note->start * daw->zoom_x - daw->scroll_x;
            int nw = note->duration * daw->zoom_x;
            
            if (ny < y - note_h || ny > y + h) continue;
            if (nx > x + w || nx + nw < grid_x) continue;
            
            color_t c = (i == daw->selected_note) ? RGB(255, 200, 100) : DAW_ACCENT;
            fb_fill_rect(nx, ny, nw - 1, note_h - 1, c);
            
            /* Velocity bar */
            int vel_h = note->velocity * (note_h - 2) / 127;
            fb_fill_rect(nx, ny + note_h - 1 - vel_h, 3, vel_h, RGB(100, 255, 100));
        }
    }
    
    /* Draw playhead */
    int playhead_x = grid_x + daw->current_tick * daw->zoom_x - daw->scroll_x;
    if (playhead_x >= grid_x && playhead_x <= x + w) {
        fb_draw_line(playhead_x, y, playhead_x, y + h, DAW_PLAYHEAD);
    }
}

/* Draw mixer */
static void daw_draw_mixer(daw_state_t *daw) {
    int x = daw->window->bounds.x + daw->window->client.x;
    int y = daw->window->bounds.y + daw->window->client.y + 45;
    int w = daw->window->client.width;
    int h = daw->window->client.height - 45;
    
    /* Background */
    fb_fill_rect(x, y, w, h, DAW_BG);
    
    /* Channel strip width */
    int strip_w = 60;
    
    for (int t = 0; t < daw->project.track_count; t++) {
        daw_track_t *track = &daw->project.tracks[t];
        int sx = x + t * (strip_w + 4) + 4;
        
        if (sx > x + w) break;
        
        /* Strip background */
        fb_fill_rect(sx, y + 4, strip_w, h - 8, DAW_PANEL);
        
        /* Track color */
        fb_fill_rect(sx, y + 4, strip_w, 4, track->color);
        
        /* Track name */
        fb_draw_string(sx + 4, y + 12, track->name, DAW_TEXT, 0);
        
        /* Volume fader */
        int fader_x = sx + strip_w / 2 - 8;
        int fader_y = y + 40;
        int fader_h = h - 120;
        
        fb_fill_rect(fader_x, fader_y, 16, fader_h, DAW_GRID);
        
        int vol_y = fader_y + fader_h - (track->volume * fader_h / 127);
        fb_fill_rect(fader_x, vol_y, 16, fader_y + fader_h - vol_y, track->color);
        fb_fill_rect(fader_x - 4, vol_y - 4, 24, 8, DAW_TEXT);
        
        /* Volume value */
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", track->volume);
        fb_draw_string(sx + 20, fader_y + fader_h + 4, buf, DAW_TEXT, 0);
        
        /* Pan knob */
        int pan_x = sx + strip_w / 2;
        int pan_y = y + h - 50;
        fb_fill_rect(pan_x - 12, pan_y - 12, 24, 24, DAW_GRID);
        fb_draw_string(sx + 20, pan_y - 4, "PAN", RGB(100, 100, 100), 0);
        
        /* Mute/Solo */
        fb_fill_rect(sx + 4, y + h - 28, 24, 20, track->mute ? RGB(255, 100, 100) : DAW_GRID);
        fb_draw_string(sx + 10, y + h - 24, "M", DAW_TEXT, 0);
        
        fb_fill_rect(sx + 32, y + h - 28, 24, 20, track->solo ? RGB(255, 200, 50) : DAW_GRID);
        fb_draw_string(sx + 38, y + h - 24, "S", DAW_TEXT, 0);
    }
    
    /* Master channel */
    int mx = x + w - strip_w - 8;
    fb_fill_rect(mx, y + 4, strip_w, h - 8, RGB(50, 50, 55));
    fb_draw_string(mx + 8, y + 12, "MASTER", DAW_TEXT, 0);
}

/* Draw DAW */
void daw_draw(daw_state_t *daw) {
    if (!daw || !daw->window) return;
    
    daw_draw_toolbar(daw);
    
    switch (daw->view) {
        case VIEW_ARRANGE:
            daw_draw_arrange(daw);
            break;
        case VIEW_PIANO_ROLL:
            daw_draw_piano_roll(daw);
            break;
        case VIEW_MIXER:
            daw_draw_mixer(daw);
            break;
        default:
            break;
    }
}

/* Play */
void daw_play(daw_state_t *daw) {
    daw->playing = true;
    daw->last_tick = timer_get_ticks();
}

/* Pause */
void daw_pause(daw_state_t *daw) {
    daw->playing = false;
}

/* Stop */
void daw_stop(daw_state_t *daw) {
    daw->playing = false;
    daw->recording = false;
    daw->current_pos = 0;
    daw->current_bar = 0;
    daw->current_beat = 0;
    daw->current_tick = 0;
}

/* Record */
void daw_record(daw_state_t *daw) {
    daw->recording = true;
    daw_play(daw);
}

/* Seek */
void daw_seek(daw_state_t *daw, int position) {
    daw->current_pos = position;
    daw->current_bar = position / (daw->project.time_sig_num * 4);
    daw->current_beat = (position / 4) % daw->project.time_sig_num;
    daw->current_tick = position % 4;
}

/* Update DAW */
void daw_update(daw_state_t *daw) {
    if (!daw) return;
    
    /* Handle input */
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    daw->running = false;
                    break;
                    
                case KEY_F1:
                    daw->view = VIEW_ARRANGE;
                    break;
                case KEY_F2:
                    daw->view = VIEW_PIANO_ROLL;
                    break;
                case KEY_F3:
                    daw->view = VIEW_MIXER;
                    break;
                    
                case KEY_UP:
                    if (daw->selected_track > 0) daw->selected_track--;
                    break;
                case KEY_DOWN:
                    if (daw->selected_track < daw->project.track_count - 1) daw->selected_track++;
                    break;
                case KEY_LEFT:
                    daw->scroll_x -= daw->zoom_x;
                    if (daw->scroll_x < 0) daw->scroll_x = 0;
                    break;
                case KEY_RIGHT:
                    daw->scroll_x += daw->zoom_x;
                    break;
                    
                default:
                    if (event.ascii == ' ') {
                        if (daw->playing) daw_pause(daw);
                        else daw_play(daw);
                    } else if (event.ascii == 'm' || event.ascii == 'M') {
                        daw_mute_track(daw, daw->selected_track, 
                                      !daw->project.tracks[daw->selected_track].mute);
                    } else if (event.ascii == 's' || event.ascii == 'S') {
                        daw_solo_track(daw, daw->selected_track,
                                      !daw->project.tracks[daw->selected_track].solo);
                    } else if (event.ascii == 'r' || event.ascii == 'R') {
                        if (daw->recording) daw_stop(daw);
                        else daw_record(daw);
                    }
                    break;
            }
        }
    }
    
    /* Update playhead */
    if (daw->playing) {
        uint64_t now = timer_get_ticks();
        uint64_t elapsed = now - daw->last_tick;
        
        /* Calculate ticks per beat based on BPM */
        /* At 120 BPM, 1 beat = 500ms = ~9 ticks at 18.2 Hz */
        uint64_t ticks_per_beat = (18 * 60) / daw->project.bpm;
        
        if (elapsed >= ticks_per_beat / 4) {
            daw->current_tick++;
            if (daw->current_tick >= 4) {
                daw->current_tick = 0;
                daw->current_beat++;
                if (daw->current_beat >= daw->project.time_sig_num) {
                    daw->current_beat = 0;
                    daw->current_bar++;
                    
                    if (daw->project.loop_enabled && daw->current_bar >= daw->project.loop_end) {
                        daw->current_bar = daw->project.loop_start;
                    }
                    
                    if (daw->current_bar >= daw->project.song_length) {
                        if (!daw->project.loop_enabled) {
                            daw_stop(daw);
                        }
                    }
                }
            }
            daw->current_pos = daw->current_bar * daw->project.time_sig_num * 4 +
                              daw->current_beat * 4 + daw->current_tick;
            daw->last_tick = now;
        }
    }
    
    daw_draw(daw);
}
