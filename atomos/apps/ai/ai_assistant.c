/*
 * AtomOS AI Assistant Implementation
 * Pattern matching and natural language understanding
 */

#include "ai_assistant.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/timer.h"
#include "../../kernel/drivers/video/framebuffer.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/mm/pmm.h"

/* Knowledge base */
static const ai_knowledge_t knowledge[] = {
    /* Greetings */
    {"hello", INTENT_GREETING, "Hello! How can I help you today?", NULL},
    {"hi", INTENT_GREETING, "Hi there! What can I do for you?", NULL},
    {"hey", INTENT_GREETING, "Hey! How can I assist you?", NULL},
    {"good morning", INTENT_GREETING, "Good morning! Ready to help.", NULL},
    {"good afternoon", INTENT_GREETING, "Good afternoon! What do you need?", NULL},
    {"good evening", INTENT_GREETING, "Good evening! How can I help?", NULL},
    
    /* Goodbyes */
    {"bye", INTENT_GOODBYE, "Goodbye! Have a great day!", NULL},
    {"goodbye", INTENT_GOODBYE, "See you later!", NULL},
    {"see you", INTENT_GOODBYE, "Take care!", NULL},
    {"exit", INTENT_GOODBYE, "Goodbye! Type 'exit' to close.", NULL},
    
    /* Thanks */
    {"thank", INTENT_THANKS, "You're welcome!", NULL},
    {"thanks", INTENT_THANKS, "Happy to help!", NULL},
    {"appreciate", INTENT_THANKS, "Anytime!", NULL},
    
    /* System info */
    {"what time", INTENT_TIME_DATE, NULL, "date"},
    {"current time", INTENT_TIME_DATE, NULL, "date"},
    {"date", INTENT_TIME_DATE, NULL, "date"},
    {"uptime", INTENT_SYSTEM_INFO, NULL, "uptime"},
    {"memory", INTENT_SYSTEM_INFO, NULL, "free"},
    {"ram", INTENT_SYSTEM_INFO, NULL, "free"},
    {"disk", INTENT_SYSTEM_INFO, NULL, "df"},
    {"processes", INTENT_SYSTEM_INFO, NULL, "ps"},
    {"running", INTENT_SYSTEM_INFO, NULL, "ps"},
    {"system info", INTENT_SYSTEM_INFO, NULL, "uname -a"},
    {"os version", INTENT_SYSTEM_INFO, NULL, "uname -a"},
    
    /* File operations */
    {"list files", INTENT_FILE_OPERATION, "Here are the files:", "ls"},
    {"show files", INTENT_FILE_OPERATION, "Listing directory:", "ls"},
    {"directory", INTENT_FILE_OPERATION, "Current directory:", "pwd"},
    {"where am i", INTENT_FILE_OPERATION, NULL, "pwd"},
    {"create file", INTENT_FILE_OPERATION, "Use 'touch filename' to create a file.", NULL},
    {"delete file", INTENT_FILE_OPERATION, "Use 'rm filename' to delete a file.", NULL},
    {"make folder", INTENT_FILE_OPERATION, "Use 'mkdir foldername' to create a folder.", NULL},
    {"create folder", INTENT_FILE_OPERATION, "Use 'mkdir foldername' to create a folder.", NULL},
    
    /* App launching */
    {"open terminal", INTENT_APP_LAUNCH, "Opening terminal...", "terminal"},
    {"launch terminal", INTENT_APP_LAUNCH, "Starting terminal...", "terminal"},
    {"open music", INTENT_APP_LAUNCH, "Opening music player...", "music"},
    {"play music", INTENT_APP_LAUNCH, "Starting music player...", "music"},
    {"open image", INTENT_APP_LAUNCH, "Opening image viewer...", "imageview"},
    {"view image", INTENT_APP_LAUNCH, "Starting image viewer...", "imageview"},
    
    /* Help */
    {"help", INTENT_HELP, "I can help with:\n- System information\n- File operations\n- Launching apps\n- Programming questions\n\nJust ask!", NULL},
    {"what can you do", INTENT_HELP, "I'm an AI assistant that can:\n- Answer questions about AtomOS\n- Help with file management\n- Launch applications\n- Provide system info\n- Assist with coding", NULL},
    {"commands", INTENT_HELP, "Common commands:\n- ls: list files\n- cd: change directory\n- cat: view file\n- clear: clear screen\n\nType 'help' for more!", NULL},
    
    /* Programming */
    {"how to code", INTENT_PROGRAMMING, "AtomOS supports Javi scripting!\n\nExample:\n  var x = 10\n  print(x * 2)\n\nSave as .javi file and run it.", NULL},
    {"javi", INTENT_PROGRAMMING, "Javi is AtomOS's scripting language.\n\nFeatures:\n- Variables: var x = 10\n- Functions: func name() { }\n- Loops: for/while\n- Conditions: if/else", NULL},
    {"program", INTENT_PROGRAMMING, "To write programs in AtomOS:\n1. Use the Javi language\n2. Create a .javi file\n3. Run with: javi myfile.javi", NULL},
    
    /* Calculations */
    {"calculate", INTENT_CALCULATION, "I can help with basic math. What would you like to calculate?", NULL},
    {"math", INTENT_CALCULATION, "Tell me the numbers you want to work with!", NULL},
    
    {NULL, INTENT_UNKNOWN, NULL, NULL}
};

/* String utilities */
static char *str_lower(char *dst, const char *src, size_t size) {
    size_t i;
    for (i = 0; i < size - 1 && src[i]; i++) {
        char c = src[i];
        dst[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
    }
    dst[i] = '\0';
    return dst;
}

static bool str_contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

/*
 * Initialize AI system
 */
void ai_init(void) {
    kprintf("AI Assistant initialized with %d knowledge entries\n",
            sizeof(knowledge) / sizeof(knowledge[0]) - 1);
}

void ai_shutdown(void) {
    /* Nothing to clean up */
}

/*
 * Match pattern in input
 */
int ai_match_pattern(const char *input, const char *pattern) {
    char lower_input[512];
    char lower_pattern[256];
    
    str_lower(lower_input, input, sizeof(lower_input));
    str_lower(lower_pattern, pattern, sizeof(lower_pattern));
    
    if (str_contains(lower_input, lower_pattern)) {
        return 100 - (strlen(lower_input) - strlen(lower_pattern)) * 2;
    }
    
    return 0;
}

/*
 * Detect intent from input
 */
ai_intent_t ai_detect_intent(const char *input) {
    int best_score = 0;
    ai_intent_t best_intent = INTENT_UNKNOWN;
    
    for (int i = 0; knowledge[i].pattern; i++) {
        int score = ai_match_pattern(input, knowledge[i].pattern);
        if (score > best_score) {
            best_score = score;
            best_intent = knowledge[i].intent;
        }
    }
    
    return best_intent;
}

/*
 * Get greeting based on time
 */
const char *ai_get_greeting(void) {
    uint64_t uptime = timer_get_uptime();
    int hour = (uptime / 3600) % 24;
    
    if (hour < 12) return "Good morning!";
    if (hour < 17) return "Good afternoon!";
    return "Good evening!";
}

/*
 * Get time response
 */
const char *ai_get_time_response(void) {
    static char buf[128];
    uint64_t uptime = timer_get_uptime();
    int hours = uptime / 3600;
    int mins = (uptime % 3600) / 60;
    int secs = uptime % 60;
    
    snprintf(buf, sizeof(buf), "System uptime: %d hours, %d minutes, %d seconds",
             hours, mins, secs);
    return buf;
}

/*
 * Get system info
 */
const char *ai_get_system_info(void) {
    static char buf[512];
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    
    snprintf(buf, sizeof(buf),
             "AtomOS System Information:\n"
             "- Kernel: AtomOS 1.0\n"
             "- Architecture: i686\n"
             "- Memory: %d KB free / %d KB total\n"
             "- Desktop: Javier",
             stats.free_memory / 1024, stats.total_memory / 1024);
    
    return buf;
}

/*
 * Generate response
 */
void ai_generate_response(ai_response_t *response, ai_intent_t intent,
                          const char *input, ai_context_t *ctx) {
    response->type = AI_RESPONSE_TEXT;
    response->confidence = 0;
    response->command[0] = '\0';
    
    /* Find best matching knowledge entry */
    int best_score = 0;
    const ai_knowledge_t *best_match = NULL;
    
    for (int i = 0; knowledge[i].pattern; i++) {
        if (knowledge[i].intent == intent) {
            int score = ai_match_pattern(input, knowledge[i].pattern);
            if (score > best_score) {
                best_score = score;
                best_match = &knowledge[i];
            }
        }
    }
    
    if (best_match) {
        response->confidence = best_score;
        
        if (best_match->response) {
            strncpy(response->text, best_match->response, sizeof(response->text) - 1);
        }
        
        if (best_match->command) {
            strncpy(response->command, best_match->command, sizeof(response->command) - 1);
            response->type = AI_RESPONSE_COMMAND;
        }
    }
    
    /* Special handling for certain intents */
    switch (intent) {
        case INTENT_TIME_DATE:
            strncpy(response->text, ai_get_time_response(), sizeof(response->text) - 1);
            break;
            
        case INTENT_SYSTEM_INFO:
            if (str_contains(input, "system") || str_contains(input, "info")) {
                strncpy(response->text, ai_get_system_info(), sizeof(response->text) - 1);
            }
            break;
            
        case INTENT_UNKNOWN:
            response->confidence = 20;
            snprintf(response->text, sizeof(response->text),
                    "I'm not sure what you mean by '%s'.\n"
                    "Try asking about:\n"
                    "- System information\n"
                    "- File operations\n"
                    "- Opening apps\n"
                    "Or type 'help' for more options.", input);
            break;
            
        default:
            break;
    }
    
    /* Update context */
    if (ctx) {
        ctx->last_intent = intent;
        if (ctx->history_count < 8) {
            strncpy(ctx->history[ctx->history_count], input, 511);
            ctx->history_count++;
        }
    }
}

/*
 * Process input
 */
ai_response_t ai_process(const char *input) {
    ai_response_t response = {0};
    ai_intent_t intent = ai_detect_intent(input);
    ai_generate_response(&response, intent, input, NULL);
    return response;
}

/*
 * Process with context
 */
ai_response_t ai_process_with_context(const char *input, ai_context_t *ctx) {
    ai_response_t response = {0};
    ai_intent_t intent = ai_detect_intent(input);
    ai_generate_response(&response, intent, input, ctx);
    return response;
}

/*
 * Create AI assistant window
 */
ai_assistant_t *ai_assistant_create(int x, int y, int width, int height) {
    ai_assistant_t *ai = (ai_assistant_t *)kcalloc(1, sizeof(ai_assistant_t));
    if (!ai) return NULL;
    
    ai->window = javier_window_create("AI Assistant", x, y, width, height,
                                       WIN_DEFAULT);
    if (!ai->window) {
        kfree(ai);
        return NULL;
    }
    
    ai->running = true;
    
    /* Add welcome message */
    ai->responses[0].type = AI_RESPONSE_TEXT;
    strcpy(ai->responses[0].text, 
           "🤖 Welcome to AtomOS AI Assistant!\n\n"
           "I can help you with:\n"
           "• System information\n"
           "• File operations\n"
           "• Launching applications\n"
           "• Programming in Javi\n\n"
           "Just type your question below!");
    ai->responses[0].confidence = 100;
    ai->response_count = 1;
    
    return ai;
}

/*
 * Destroy AI assistant
 */
void ai_assistant_destroy(ai_assistant_t *ai) {
    if (ai) {
        if (ai->window) javier_window_destroy(ai->window);
        kfree(ai);
    }
}

/*
 * Draw message bubble
 */
static void draw_bubble(int x, int y, int width, int height, 
                        color_t bg, color_t border, bool is_user) {
    (void)border;  /* Not used in simple implementation */
    /* Rounded rectangle */
    fb_fill_rect(x + 4, y, width - 8, height, bg);
    fb_fill_rect(x, y + 4, width, height - 8, bg);
    
    /* Corners */
    fb_fill_circle(x + 4, y + 4, 4, bg);
    fb_fill_circle(x + width - 5, y + 4, 4, bg);
    fb_fill_circle(x + 4, y + height - 5, 4, bg);
    fb_fill_circle(x + width - 5, y + height - 5, 4, bg);
    
    /* Tail */
    if (is_user) {
        fb_fill_rect(x + width - 10, y + height - 5, 10, 5, bg);
    } else {
        fb_fill_rect(x, y + height - 5, 10, 5, bg);
    }
}

/*
 * Draw AI assistant
 */
void ai_assistant_draw(ai_assistant_t *ai) {
    if (!ai || !ai->window) return;
    
    int cx = ai->window->bounds.x + ai->window->client.x;
    int cy = ai->window->bounds.y + ai->window->client.y;
    int cw = ai->window->client.width;
    int ch = ai->window->client.height;
    
    /* Background */
    fb_fill_rect(cx, cy, cw, ch, RGB(25, 28, 35));
    
    /* Chat area */
    int chat_y = cy + 10;
    int input_height = 50;
    
    for (int i = 0; i < ai->response_count && chat_y < cy + ch - input_height - 20; i++) {
        ai_response_t *resp = &ai->responses[i];
        
        /* Calculate bubble size */
        int text_height = 20;
        int lines = 1;
        for (const char *p = resp->text; *p; p++) {
            if (*p == '\n') lines++;
        }
        text_height = lines * 18 + 20;
        
        int bubble_w = cw - 60;
        
        /* Draw bubble */
        draw_bubble(cx + 10, chat_y, bubble_w, text_height,
                    RGB(40, 45, 55), RGB(60, 70, 85), false);
        
        /* Draw text */
        int text_y = chat_y + 10;
        char *line = resp->text;
        char *next;
        while (line && *line && text_y < chat_y + text_height - 10) {
            next = strchr(line, '\n');
            size_t len = next ? (size_t)(next - line) : strlen(line);
            char buf[256];
            if (len > 255) len = 255;
            strncpy(buf, line, len);
            buf[len] = '\0';
            
            fb_draw_string(cx + 20, text_y, buf, RGB(220, 225, 235), 0);
            text_y += 18;
            line = next ? next + 1 : NULL;
        }
        
        chat_y += text_height + 10;
    }
    
    /* Input area */
    int input_y = cy + ch - input_height;
    fb_fill_rect(cx, input_y, cw, input_height, RGB(35, 40, 50));
    fb_draw_line(cx, input_y, cx + cw, input_y, RGB(50, 55, 65));
    
    /* Input box */
    fb_fill_rect(cx + 10, input_y + 10, cw - 90, 30, RGB(25, 28, 35));
    fb_draw_rect(cx + 10, input_y + 10, cw - 90, 30, RGB(60, 70, 85));
    
    /* Input text */
    fb_draw_string(cx + 15, input_y + 17, ai->input, RGB(220, 225, 235), 0);
    
    /* Cursor */
    int cursor_x = cx + 15 + ai->input_pos * 8;
    if ((timer_get_ticks() / 50) % 2 == 0) {
        fb_fill_rect(cursor_x, input_y + 14, 2, 18, RGB(100, 150, 255));
    }
    
    /* Send button */
    fb_fill_rect(cx + cw - 70, input_y + 10, 60, 30, RGB(45, 125, 210));
    fb_draw_string(cx + cw - 55, input_y + 17, "Send", RGB(255, 255, 255), 0);
    
    /* AI indicator */
    if (ai->processing) {
        fb_draw_string(cx + 10, cy + 5, "🤔 Thinking...", RGB(150, 160, 180), 0);
    }
}

/*
 * Send message
 */
void ai_assistant_send_message(ai_assistant_t *ai, const char *message) {
    if (!ai || !message || !*message) return;
    
    ai->processing = true;
    
    /* Add user message */
    if (ai->response_count < 19) {
        ai->responses[ai->response_count].type = AI_RESPONSE_TEXT;
        snprintf(ai->responses[ai->response_count].text, 
                 sizeof(ai->responses[ai->response_count].text),
                 "You: %s", message);
        ai->responses[ai->response_count].confidence = 100;
        ai->response_count++;
    }
    
    /* Process and add AI response */
    ai_response_t response = ai_process_with_context(message, &ai->context);
    
    if (ai->response_count < 20) {
        ai->responses[ai->response_count] = response;
        ai->response_count++;
    }
    
    ai->processing = false;
}

/*
 * Update AI assistant
 */
void ai_assistant_update(ai_assistant_t *ai) {
    if (!ai || !ai->running) return;
    
    if (keyboard_has_key()) {
        key_event_t event = keyboard_get_key();
        if (event.pressed) {
            switch (event.scancode) {
                case KEY_ESC:
                    ai->running = false;
                    break;
                    
                case KEY_ENTER:
                    if (ai->input_pos > 0) {
                        ai_assistant_send_message(ai, ai->input);
                        ai->input[0] = '\0';
                        ai->input_pos = 0;
                    }
                    break;
                    
                case KEY_BACKSPACE:
                    if (ai->input_pos > 0) {
                        ai->input_pos--;
                        ai->input[ai->input_pos] = '\0';
                    }
                    break;
                    
                default:
                    if (event.ascii >= ' ' && ai->input_pos < 510) {
                        ai->input[ai->input_pos++] = event.ascii;
                        ai->input[ai->input_pos] = '\0';
                    }
                    break;
            }
        }
    }
}

/*
 * Execute command from AI
 */
bool ai_execute_command(const char *command) {
    if (!command || !*command) return false;
    
    kprintf("AI executing: %s\n", command);
    /* Would call shell to execute */
    return true;
}

/*
 * Suggest command from description
 */
char *ai_suggest_command(const char *description) {
    static char cmd[256];
    
    if (str_contains(description, "list") && str_contains(description, "file")) {
        strcpy(cmd, "ls -la");
    } else if (str_contains(description, "current") && str_contains(description, "dir")) {
        strcpy(cmd, "pwd");
    } else if (str_contains(description, "memory")) {
        strcpy(cmd, "free");
    } else if (str_contains(description, "process")) {
        strcpy(cmd, "ps");
    } else {
        cmd[0] = '\0';
    }
    
    return cmd[0] ? cmd : NULL;
}
