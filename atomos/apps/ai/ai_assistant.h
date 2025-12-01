/*
 * AtomOS AI Assistant
 * Pattern-matching based assistant for help and automation
 */

#ifndef _ATOMOS_AI_ASSISTANT_H
#define _ATOMOS_AI_ASSISTANT_H

#include "../../kernel/include/types.h"
#include "../../javier/core/javier.h"

/* AI response types */
typedef enum {
    AI_RESPONSE_TEXT,
    AI_RESPONSE_COMMAND,
    AI_RESPONSE_CODE,
    AI_RESPONSE_HELP,
    AI_RESPONSE_ERROR
} ai_response_type_t;

/* Intent categories */
typedef enum {
    INTENT_UNKNOWN,
    INTENT_HELP,
    INTENT_FILE_OPERATION,
    INTENT_SYSTEM_INFO,
    INTENT_APP_LAUNCH,
    INTENT_SETTINGS,
    INTENT_PROGRAMMING,
    INTENT_CALCULATION,
    INTENT_TIME_DATE,
    INTENT_WEATHER,
    INTENT_GREETING,
    INTENT_GOODBYE,
    INTENT_THANKS,
    INTENT_SEARCH
} ai_intent_t;

/* AI response structure */
typedef struct {
    ai_response_type_t type;
    char text[1024];
    char command[256];
    int confidence;  /* 0-100 */
} ai_response_t;

/* Conversation context */
typedef struct {
    char history[8][512];
    int history_count;
    ai_intent_t last_intent;
    char last_topic[64];
    bool awaiting_confirmation;
} ai_context_t;

/* Knowledge entry */
typedef struct {
    const char *pattern;
    ai_intent_t intent;
    const char *response;
    const char *command;
} ai_knowledge_t;

/* AI Assistant */
typedef struct {
    ai_context_t context;
    javier_window_t *window;
    char input[512];
    int input_pos;
    ai_response_t responses[20];
    int response_count;
    bool running;
    bool processing;
} ai_assistant_t;

/* Initialize/cleanup */
void ai_init(void);
void ai_shutdown(void);

/* Process input */
ai_response_t ai_process(const char *input);
ai_response_t ai_process_with_context(const char *input, ai_context_t *ctx);

/* Intent detection */
ai_intent_t ai_detect_intent(const char *input);
int ai_match_pattern(const char *input, const char *pattern);

/* Response generation */
void ai_generate_response(ai_response_t *response, ai_intent_t intent,
                          const char *input, ai_context_t *ctx);

/* Helper functions */
const char *ai_get_greeting(void);
const char *ai_get_time_response(void);
const char *ai_get_system_info(void);

/* GUI Assistant */
ai_assistant_t *ai_assistant_create(int x, int y, int width, int height);
void ai_assistant_destroy(ai_assistant_t *ai);
void ai_assistant_draw(ai_assistant_t *ai);
void ai_assistant_update(ai_assistant_t *ai);
void ai_assistant_send_message(ai_assistant_t *ai, const char *message);

/* Quick commands */
bool ai_execute_command(const char *command);
char *ai_suggest_command(const char *description);

#endif /* _ATOMOS_AI_ASSISTANT_H */
