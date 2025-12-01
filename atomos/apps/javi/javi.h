/*
 * Javi Programming Language
 * A simple scripting language for AtomOS
 * 
 * Syntax example:
 *   # Comment
 *   var x = 10
 *   var name = "Hello"
 *   
 *   func greet(name) {
 *       print("Hello, " + name)
 *   }
 *   
 *   if x > 5 {
 *       greet("World")
 *   }
 *   
 *   for i in 0..10 {
 *       print(i)
 *   }
 */

#ifndef _JAVI_H
#define _JAVI_H

#include "../../kernel/include/types.h"

/* Limits */
#define JAVI_MAX_STRING     256
#define JAVI_MAX_VARS       256
#define JAVI_MAX_FUNCS      64
#define JAVI_MAX_STACK      256
#define JAVI_MAX_CODE       4096

/* Token types */
typedef enum {
    TOK_EOF = 0,
    TOK_ERROR,
    
    /* Literals */
    TOK_NUMBER,
    TOK_STRING,
    TOK_IDENTIFIER,
    
    /* Keywords */
    TOK_VAR,
    TOK_FUNC,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_RETURN,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_PRINT,
    TOK_INPUT,
    
    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_EQUAL,
    TOK_EQUALEQUAL,
    TOK_NOTEQUAL,
    TOK_LESS,
    TOK_LESSEQUAL,
    TOK_GREATER,
    TOK_GREATEREQUAL,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_DOTDOT,
    
    /* Delimiters */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_NEWLINE,
} javi_token_type_t;

/* Value types */
typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_ARRAY,
    VAL_FUNC,
} javi_value_type_t;

/* Forward declarations */
struct javi_value;
struct javi_func;

/* Value structure */
typedef struct javi_value {
    javi_value_type_t type;
    union {
        bool boolean;
        int32_t number;
        char string[JAVI_MAX_STRING];
        struct {
            struct javi_value *items;
            int count;
        } array;
        struct javi_func *func;
    };
} javi_value_t;

/* Variable */
typedef struct {
    char name[64];
    javi_value_t value;
} javi_var_t;

/* Function */
typedef struct javi_func {
    char name[64];
    char params[8][64];
    int param_count;
    char *body;
    int body_len;
    bool builtin;
    javi_value_t (*native)(int argc, javi_value_t *argv);
} javi_func_t;

/* Token */
typedef struct {
    javi_token_type_t type;
    char text[JAVI_MAX_STRING];
    int32_t number;
    int line;
    int col;
} javi_token_t;

/* Lexer state */
typedef struct {
    const char *source;
    const char *current;
    int line;
    int col;
    javi_token_t token;
} javi_lexer_t;

/* Virtual machine state */
typedef struct {
    javi_var_t vars[JAVI_MAX_VARS];
    int var_count;
    javi_func_t funcs[JAVI_MAX_FUNCS];
    int func_count;
    javi_value_t stack[JAVI_MAX_STACK];
    int stack_top;
    javi_lexer_t lexer;
    char *output;
    int output_len;
    int output_cap;
    bool error;
    char error_msg[256];
} javi_vm_t;

/* Compiler functions */
javi_vm_t *javi_create(void);
void javi_destroy(javi_vm_t *vm);
bool javi_compile(javi_vm_t *vm, const char *source);
bool javi_execute(javi_vm_t *vm, const char *source);
const char *javi_get_output(javi_vm_t *vm);
const char *javi_get_error(javi_vm_t *vm);

/* Value operations */
javi_value_t javi_null(void);
javi_value_t javi_bool(bool value);
javi_value_t javi_number(int32_t value);
javi_value_t javi_string(const char *value);

/* Standard library */
void javi_register_stdlib(javi_vm_t *vm);

/* Variable/Function operations */
void javi_set_var(javi_vm_t *vm, const char *name, javi_value_t value);
javi_value_t *javi_get_var(javi_vm_t *vm, const char *name);
void javi_register_func(javi_vm_t *vm, const char *name, 
                        javi_value_t (*func)(int, javi_value_t*));

#endif /* _JAVI_H */
