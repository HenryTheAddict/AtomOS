/*
 * Javi Programming Language Implementation
 * Lexer, parser, and interpreter for AtomOS
 */

#include "javi.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Keywords table */
static const struct {
    const char *word;
    javi_token_type_t type;
} keywords[] = {
    {"var", TOK_VAR},
    {"func", TOK_FUNC},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"while", TOK_WHILE},
    {"for", TOK_FOR},
    {"in", TOK_IN},
    {"return", TOK_RETURN},
    {"true", TOK_TRUE},
    {"false", TOK_FALSE},
    {"null", TOK_NULL},
    {"print", TOK_PRINT},
    {"input", TOK_INPUT},
    {"and", TOK_AND},
    {"or", TOK_OR},
    {"not", TOK_NOT},
    {NULL, TOK_EOF}
};

/* Value constructors */
javi_value_t javi_null(void) {
    javi_value_t v;
    v.type = VAL_NULL;
    return v;
}

javi_value_t javi_bool(bool value) {
    javi_value_t v;
    v.type = VAL_BOOL;
    v.boolean = value;
    return v;
}

javi_value_t javi_number(int32_t value) {
    javi_value_t v;
    v.type = VAL_NUMBER;
    v.number = value;
    return v;
}

javi_value_t javi_string(const char *value) {
    javi_value_t v;
    v.type = VAL_STRING;
    strncpy(v.string, value, JAVI_MAX_STRING - 1);
    return v;
}

/* Create VM */
javi_vm_t *javi_create(void) {
    javi_vm_t *vm = (javi_vm_t *)kcalloc(1, sizeof(javi_vm_t));
    if (!vm) return NULL;
    
    vm->output_cap = 4096;
    vm->output = (char *)kmalloc(vm->output_cap);
    vm->output[0] = '\0';
    
    javi_register_stdlib(vm);
    
    return vm;
}

/* Destroy VM */
void javi_destroy(javi_vm_t *vm) {
    if (vm) {
        if (vm->output) kfree(vm->output);
        kfree(vm);
    }
}

/* Append to output */
static void vm_output(javi_vm_t *vm, const char *str) {
    int len = strlen(str);
    while (vm->output_len + len + 1 > vm->output_cap) {
        vm->output_cap *= 2;
        vm->output = krealloc(vm->output, vm->output_cap);
    }
    strcpy(vm->output + vm->output_len, str);
    vm->output_len += len;
}

/* Set error */
static void vm_error(javi_vm_t *vm, const char *msg) {
    vm->error = true;
    strncpy(vm->error_msg, msg, sizeof(vm->error_msg) - 1);
}

/* ========== LEXER ========== */

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static char peek(javi_lexer_t *lex) {
    return *lex->current;
}

static char advance(javi_lexer_t *lex) {
    char c = *lex->current++;
    if (c == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }
    return c;
}

static bool match(javi_lexer_t *lex, char expected) {
    if (*lex->current != expected) return false;
    advance(lex);
    return true;
}

static void skip_whitespace(javi_lexer_t *lex) {
    while (1) {
        char c = peek(lex);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance(lex);
                break;
            case '#':
                /* Skip comment to end of line */
                while (peek(lex) && peek(lex) != '\n') advance(lex);
                break;
            default:
                return;
        }
    }
}

static javi_token_t make_token(javi_lexer_t *lex, javi_token_type_t type) {
    javi_token_t tok;
    tok.type = type;
    tok.text[0] = '\0';
    tok.number = 0;
    tok.line = lex->line;
    tok.col = lex->col;
    return tok;
}

static javi_token_t number_token(javi_lexer_t *lex) {
    const char *start = lex->current - 1;
    while (is_digit(peek(lex))) advance(lex);
    
    javi_token_t tok = make_token(lex, TOK_NUMBER);
    
    int len = lex->current - start;
    if (len >= JAVI_MAX_STRING) len = JAVI_MAX_STRING - 1;
    strncpy(tok.text, start, len);
    tok.text[len] = '\0';
    
    /* Parse number */
    tok.number = 0;
    for (const char *p = start; p < lex->current; p++) {
        tok.number = tok.number * 10 + (*p - '0');
    }
    
    return tok;
}

static javi_token_t string_token(javi_lexer_t *lex, char quote) {
    const char *start = lex->current;
    
    while (peek(lex) && peek(lex) != quote && peek(lex) != '\n') {
        advance(lex);
    }
    
    javi_token_t tok = make_token(lex, TOK_STRING);
    
    int len = lex->current - start;
    if (len >= JAVI_MAX_STRING) len = JAVI_MAX_STRING - 1;
    strncpy(tok.text, start, len);
    tok.text[len] = '\0';
    
    if (peek(lex) == quote) advance(lex);
    
    return tok;
}

static javi_token_t identifier_token(javi_lexer_t *lex) {
    const char *start = lex->current - 1;
    while (is_alnum(peek(lex))) advance(lex);
    
    javi_token_t tok = make_token(lex, TOK_IDENTIFIER);
    
    int len = lex->current - start;
    if (len >= JAVI_MAX_STRING) len = JAVI_MAX_STRING - 1;
    strncpy(tok.text, start, len);
    tok.text[len] = '\0';
    
    /* Check for keywords */
    for (int i = 0; keywords[i].word; i++) {
        if (strcmp(tok.text, keywords[i].word) == 0) {
            tok.type = keywords[i].type;
            break;
        }
    }
    
    return tok;
}

static javi_token_t next_token(javi_lexer_t *lex) {
    skip_whitespace(lex);
    
    if (!peek(lex)) {
        return make_token(lex, TOK_EOF);
    }
    
    char c = advance(lex);
    
    if (is_digit(c)) return number_token(lex);
    if (is_alpha(c)) return identifier_token(lex);
    if (c == '"' || c == '\'') return string_token(lex, c);
    
    switch (c) {
        case '\n': return make_token(lex, TOK_NEWLINE);
        case '(': return make_token(lex, TOK_LPAREN);
        case ')': return make_token(lex, TOK_RPAREN);
        case '{': return make_token(lex, TOK_LBRACE);
        case '}': return make_token(lex, TOK_RBRACE);
        case '[': return make_token(lex, TOK_LBRACKET);
        case ']': return make_token(lex, TOK_RBRACKET);
        case ',': return make_token(lex, TOK_COMMA);
        case ':': return make_token(lex, TOK_COLON);
        case ';': return make_token(lex, TOK_SEMICOLON);
        case '+': return make_token(lex, TOK_PLUS);
        case '-': return make_token(lex, TOK_MINUS);
        case '*': return make_token(lex, TOK_STAR);
        case '/': return make_token(lex, TOK_SLASH);
        case '%': return make_token(lex, TOK_PERCENT);
        case '.':
            if (match(lex, '.')) return make_token(lex, TOK_DOTDOT);
            break;
        case '=':
            if (match(lex, '=')) return make_token(lex, TOK_EQUALEQUAL);
            return make_token(lex, TOK_EQUAL);
        case '!':
            if (match(lex, '=')) return make_token(lex, TOK_NOTEQUAL);
            return make_token(lex, TOK_NOT);
        case '<':
            if (match(lex, '=')) return make_token(lex, TOK_LESSEQUAL);
            return make_token(lex, TOK_LESS);
        case '>':
            if (match(lex, '=')) return make_token(lex, TOK_GREATEREQUAL);
            return make_token(lex, TOK_GREATER);
        case '&':
            if (match(lex, '&')) return make_token(lex, TOK_AND);
            break;
        case '|':
            if (match(lex, '|')) return make_token(lex, TOK_OR);
            break;
    }
    
    javi_token_t tok = make_token(lex, TOK_ERROR);
    tok.text[0] = c;
    tok.text[1] = '\0';
    return tok;
}

/* ========== PARSER & INTERPRETER ========== */

static void next(javi_vm_t *vm) {
    vm->lexer.token = next_token(&vm->lexer);
}

static bool check(javi_vm_t *vm, javi_token_type_t type) {
    return vm->lexer.token.type == type;
}

static bool consume(javi_vm_t *vm, javi_token_type_t type) {
    if (check(vm, type)) {
        next(vm);
        return true;
    }
    return false;
}

static void skip_newlines(javi_vm_t *vm) {
    while (check(vm, TOK_NEWLINE)) next(vm);
}

/* Variable operations */
void javi_set_var(javi_vm_t *vm, const char *name, javi_value_t value) {
    /* Check if exists */
    for (int i = 0; i < vm->var_count; i++) {
        if (strcmp(vm->vars[i].name, name) == 0) {
            vm->vars[i].value = value;
            return;
        }
    }
    
    /* Add new */
    if (vm->var_count < JAVI_MAX_VARS) {
        strncpy(vm->vars[vm->var_count].name, name, 63);
        vm->vars[vm->var_count].value = value;
        vm->var_count++;
    }
}

javi_value_t *javi_get_var(javi_vm_t *vm, const char *name) {
    for (int i = 0; i < vm->var_count; i++) {
        if (strcmp(vm->vars[i].name, name) == 0) {
            return &vm->vars[i].value;
        }
    }
    return NULL;
}

/* Function operations */
void javi_register_func(javi_vm_t *vm, const char *name, 
                        javi_value_t (*func)(int, javi_value_t*)) {
    if (vm->func_count < JAVI_MAX_FUNCS) {
        strncpy(vm->funcs[vm->func_count].name, name, 63);
        vm->funcs[vm->func_count].builtin = true;
        vm->funcs[vm->func_count].native = func;
        vm->func_count++;
    }
}

static javi_func_t *get_func(javi_vm_t *vm, const char *name) {
    for (int i = 0; i < vm->func_count; i++) {
        if (strcmp(vm->funcs[i].name, name) == 0) {
            return &vm->funcs[i];
        }
    }
    return NULL;
}

/* Forward declarations */
static javi_value_t parse_expr(javi_vm_t *vm);
static void parse_statement(javi_vm_t *vm);
static void parse_block(javi_vm_t *vm);

/* Value to string */
static void value_to_string(javi_value_t *val, char *buf, int size) {
    switch (val->type) {
        case VAL_NULL:
            strncpy(buf, "null", size);
            break;
        case VAL_BOOL:
            strncpy(buf, val->boolean ? "true" : "false", size);
            break;
        case VAL_NUMBER: {
            int n = val->number;
            char *p = buf + size - 1;
            *p = '\0';
            bool neg = n < 0;
            if (neg) n = -n;
            do {
                *--p = '0' + n % 10;
                n /= 10;
            } while (n && p > buf);
            if (neg && p > buf) *--p = '-';
            memmove(buf, p, strlen(p) + 1);
            break;
        }
        case VAL_STRING:
            strncpy(buf, val->string, size);
            break;
        default:
            strncpy(buf, "<value>", size);
            break;
    }
}

/* Value is truthy */
static bool is_truthy(javi_value_t *val) {
    switch (val->type) {
        case VAL_NULL: return false;
        case VAL_BOOL: return val->boolean;
        case VAL_NUMBER: return val->number != 0;
        case VAL_STRING: return val->string[0] != '\0';
        default: return true;
    }
}

/* Parse primary expression */
static javi_value_t parse_primary(javi_vm_t *vm) {
    javi_token_t tok = vm->lexer.token;
    
    switch (tok.type) {
        case TOK_NUMBER:
            next(vm);
            return javi_number(tok.number);
            
        case TOK_STRING:
            next(vm);
            return javi_string(tok.text);
            
        case TOK_TRUE:
            next(vm);
            return javi_bool(true);
            
        case TOK_FALSE:
            next(vm);
            return javi_bool(false);
            
        case TOK_NULL:
            next(vm);
            return javi_null();
            
        case TOK_IDENTIFIER: {
            char name[64];
            strncpy(name, tok.text, 63);
            next(vm);
            
            /* Function call? */
            if (check(vm, TOK_LPAREN)) {
                next(vm);
                
                javi_value_t args[8];
                int argc = 0;
                
                while (!check(vm, TOK_RPAREN) && argc < 8) {
                    args[argc++] = parse_expr(vm);
                    if (!consume(vm, TOK_COMMA)) break;
                }
                consume(vm, TOK_RPAREN);
                
                javi_func_t *func = get_func(vm, name);
                if (func && func->builtin && func->native) {
                    return func->native(argc, args);
                }
                
                vm_error(vm, "Unknown function");
                return javi_null();
            }
            
            /* Variable */
            javi_value_t *var = javi_get_var(vm, name);
            if (var) return *var;
            
            vm_error(vm, "Undefined variable");
            return javi_null();
        }
        
        case TOK_LPAREN:
            next(vm);
            {
                javi_value_t val = parse_expr(vm);
                consume(vm, TOK_RPAREN);
                return val;
            }
            
        default:
            next(vm);
            return javi_null();
    }
}

/* Parse unary */
static javi_value_t parse_unary(javi_vm_t *vm) {
    if (check(vm, TOK_MINUS)) {
        next(vm);
        javi_value_t val = parse_unary(vm);
        if (val.type == VAL_NUMBER) val.number = -val.number;
        return val;
    }
    if (check(vm, TOK_NOT)) {
        next(vm);
        javi_value_t val = parse_unary(vm);
        return javi_bool(!is_truthy(&val));
    }
    return parse_primary(vm);
}

/* Parse multiplicative */
static javi_value_t parse_multiplicative(javi_vm_t *vm) {
    javi_value_t left = parse_unary(vm);
    
    while (check(vm, TOK_STAR) || check(vm, TOK_SLASH) || check(vm, TOK_PERCENT)) {
        javi_token_type_t op = vm->lexer.token.type;
        next(vm);
        javi_value_t right = parse_unary(vm);
        
        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
            switch (op) {
                case TOK_STAR: left.number *= right.number; break;
                case TOK_SLASH: 
                    if (right.number != 0) left.number /= right.number; 
                    break;
                case TOK_PERCENT: 
                    if (right.number != 0) left.number %= right.number; 
                    break;
                default: break;
            }
        }
    }
    
    return left;
}

/* Parse additive */
static javi_value_t parse_additive(javi_vm_t *vm) {
    javi_value_t left = parse_multiplicative(vm);
    
    while (check(vm, TOK_PLUS) || check(vm, TOK_MINUS)) {
        javi_token_type_t op = vm->lexer.token.type;
        next(vm);
        javi_value_t right = parse_multiplicative(vm);
        
        if (op == TOK_PLUS && left.type == VAL_STRING) {
            /* String concatenation */
            char buf[JAVI_MAX_STRING];
            value_to_string(&right, buf, sizeof(buf));
            strncat(left.string, buf, JAVI_MAX_STRING - strlen(left.string) - 1);
        } else if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
            if (op == TOK_PLUS) left.number += right.number;
            else left.number -= right.number;
        }
    }
    
    return left;
}

/* Parse comparison */
static javi_value_t parse_comparison(javi_vm_t *vm) {
    javi_value_t left = parse_additive(vm);
    
    while (check(vm, TOK_LESS) || check(vm, TOK_LESSEQUAL) ||
           check(vm, TOK_GREATER) || check(vm, TOK_GREATEREQUAL)) {
        javi_token_type_t op = vm->lexer.token.type;
        next(vm);
        javi_value_t right = parse_additive(vm);
        
        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
            bool result = false;
            switch (op) {
                case TOK_LESS: result = left.number < right.number; break;
                case TOK_LESSEQUAL: result = left.number <= right.number; break;
                case TOK_GREATER: result = left.number > right.number; break;
                case TOK_GREATEREQUAL: result = left.number >= right.number; break;
                default: break;
            }
            left = javi_bool(result);
        }
    }
    
    return left;
}

/* Parse equality */
static javi_value_t parse_equality(javi_vm_t *vm) {
    javi_value_t left = parse_comparison(vm);
    
    while (check(vm, TOK_EQUALEQUAL) || check(vm, TOK_NOTEQUAL)) {
        javi_token_type_t op = vm->lexer.token.type;
        next(vm);
        javi_value_t right = parse_comparison(vm);
        
        bool equal = false;
        if (left.type == right.type) {
            switch (left.type) {
                case VAL_NULL: equal = true; break;
                case VAL_BOOL: equal = left.boolean == right.boolean; break;
                case VAL_NUMBER: equal = left.number == right.number; break;
                case VAL_STRING: equal = strcmp(left.string, right.string) == 0; break;
                default: break;
            }
        }
        
        left = javi_bool(op == TOK_EQUALEQUAL ? equal : !equal);
    }
    
    return left;
}

/* Parse logical and */
static javi_value_t parse_and(javi_vm_t *vm) {
    javi_value_t left = parse_equality(vm);
    
    while (check(vm, TOK_AND)) {
        next(vm);
        if (!is_truthy(&left)) return left;
        left = parse_equality(vm);
    }
    
    return left;
}

/* Parse logical or */
static javi_value_t parse_or(javi_vm_t *vm) {
    javi_value_t left = parse_and(vm);
    
    while (check(vm, TOK_OR)) {
        next(vm);
        if (is_truthy(&left)) return left;
        left = parse_and(vm);
    }
    
    return left;
}

/* Parse expression */
static javi_value_t parse_expr(javi_vm_t *vm) {
    return parse_or(vm);
}

/* Parse variable declaration */
static void parse_var_decl(javi_vm_t *vm) {
    next(vm);  /* Consume 'var' */
    
    if (!check(vm, TOK_IDENTIFIER)) {
        vm_error(vm, "Expected variable name");
        return;
    }
    
    char name[64];
    strncpy(name, vm->lexer.token.text, 63);
    next(vm);
    
    javi_value_t value = javi_null();
    
    if (consume(vm, TOK_EQUAL)) {
        value = parse_expr(vm);
    }
    
    javi_set_var(vm, name, value);
}

/* Parse if statement */
static void parse_if(javi_vm_t *vm) {
    next(vm);  /* Consume 'if' */
    
    javi_value_t cond = parse_expr(vm);
    skip_newlines(vm);
    
    if (is_truthy(&cond)) {
        parse_block(vm);
        skip_newlines(vm);
        if (check(vm, TOK_ELSE)) {
            next(vm);
            skip_newlines(vm);
            /* Skip else block */
            if (check(vm, TOK_LBRACE)) {
                int depth = 1;
                next(vm);
                while (depth > 0 && !check(vm, TOK_EOF)) {
                    if (check(vm, TOK_LBRACE)) depth++;
                    if (check(vm, TOK_RBRACE)) depth--;
                    next(vm);
                }
            }
        }
    } else {
        /* Skip if block */
        if (check(vm, TOK_LBRACE)) {
            int depth = 1;
            next(vm);
            while (depth > 0 && !check(vm, TOK_EOF)) {
                if (check(vm, TOK_LBRACE)) depth++;
                if (check(vm, TOK_RBRACE)) depth--;
                next(vm);
            }
        }
        skip_newlines(vm);
        if (check(vm, TOK_ELSE)) {
            next(vm);
            skip_newlines(vm);
            if (check(vm, TOK_IF)) {
                parse_if(vm);
            } else {
                parse_block(vm);
            }
        }
    }
}

/* Parse while statement */
static void parse_while(javi_vm_t *vm) {
    next(vm);  /* Consume 'while' */
    
    const char *cond_start = vm->lexer.current;
    javi_value_t cond = parse_expr(vm);
    const char *body_start = vm->lexer.current;
    
    while (is_truthy(&cond) && !vm->error) {
        skip_newlines(vm);
        parse_block(vm);
        
        /* Re-evaluate condition */
        vm->lexer.current = cond_start;
        next(vm);
        cond = parse_expr(vm);
        vm->lexer.current = body_start;
    }
    
    /* Skip body if condition is false */
    skip_newlines(vm);
    if (check(vm, TOK_LBRACE)) {
        int depth = 1;
        next(vm);
        while (depth > 0 && !check(vm, TOK_EOF)) {
            if (check(vm, TOK_LBRACE)) depth++;
            if (check(vm, TOK_RBRACE)) depth--;
            next(vm);
        }
    }
}

/* Parse for statement */
static void parse_for(javi_vm_t *vm) {
    next(vm);  /* Consume 'for' */
    
    if (!check(vm, TOK_IDENTIFIER)) {
        vm_error(vm, "Expected variable in for loop");
        return;
    }
    
    char var_name[64];
    strncpy(var_name, vm->lexer.token.text, 63);
    next(vm);
    
    if (!consume(vm, TOK_IN)) {
        vm_error(vm, "Expected 'in' in for loop");
        return;
    }
    
    javi_value_t start = parse_expr(vm);
    
    if (!consume(vm, TOK_DOTDOT)) {
        vm_error(vm, "Expected '..' in range");
        return;
    }
    
    javi_value_t end = parse_expr(vm);
    
    if (start.type != VAL_NUMBER || end.type != VAL_NUMBER) {
        vm_error(vm, "Range must be numeric");
        return;
    }
    
    skip_newlines(vm);
    const char *body_start = vm->lexer.current;
    
    for (int32_t i = start.number; i < end.number && !vm->error; i++) {
        javi_set_var(vm, var_name, javi_number(i));
        vm->lexer.current = body_start;
        next(vm);
        parse_block(vm);
    }
    
    /* Position after loop body */
    vm->lexer.current = body_start;
    next(vm);
    if (check(vm, TOK_LBRACE)) {
        int depth = 1;
        next(vm);
        while (depth > 0 && !check(vm, TOK_EOF)) {
            if (check(vm, TOK_LBRACE)) depth++;
            if (check(vm, TOK_RBRACE)) depth--;
            next(vm);
        }
    }
}

/* Parse print statement */
static void parse_print(javi_vm_t *vm) {
    next(vm);  /* Consume 'print' */
    consume(vm, TOK_LPAREN);
    
    javi_value_t val = parse_expr(vm);
    char buf[JAVI_MAX_STRING];
    value_to_string(&val, buf, sizeof(buf));
    vm_output(vm, buf);
    vm_output(vm, "\n");
    
    consume(vm, TOK_RPAREN);
}

/* Parse block */
static void parse_block(javi_vm_t *vm) {
    if (!consume(vm, TOK_LBRACE)) {
        parse_statement(vm);
        return;
    }
    
    skip_newlines(vm);
    while (!check(vm, TOK_RBRACE) && !check(vm, TOK_EOF) && !vm->error) {
        parse_statement(vm);
        skip_newlines(vm);
    }
    consume(vm, TOK_RBRACE);
}

/* Parse statement */
static void parse_statement(javi_vm_t *vm) {
    skip_newlines(vm);
    
    switch (vm->lexer.token.type) {
        case TOK_VAR:
            parse_var_decl(vm);
            break;
        case TOK_IF:
            parse_if(vm);
            break;
        case TOK_WHILE:
            parse_while(vm);
            break;
        case TOK_FOR:
            parse_for(vm);
            break;
        case TOK_PRINT:
            parse_print(vm);
            break;
        case TOK_LBRACE:
            parse_block(vm);
            break;
        case TOK_IDENTIFIER: {
            char name[64];
            strncpy(name, vm->lexer.token.text, 63);
            next(vm);
            if (consume(vm, TOK_EQUAL)) {
                javi_value_t val = parse_expr(vm);
                javi_set_var(vm, name, val);
            } else if (check(vm, TOK_LPAREN)) {
                /* Function call as statement */
                next(vm);
                javi_value_t args[8];
                int argc = 0;
                while (!check(vm, TOK_RPAREN) && argc < 8) {
                    args[argc++] = parse_expr(vm);
                    if (!consume(vm, TOK_COMMA)) break;
                }
                consume(vm, TOK_RPAREN);
                
                javi_func_t *func = get_func(vm, name);
                if (func && func->builtin && func->native) {
                    func->native(argc, args);
                }
            }
            break;
        }
        default:
            next(vm);
            break;
    }
}

/* Execute source code */
bool javi_execute(javi_vm_t *vm, const char *source) {
    vm->error = false;
    vm->error_msg[0] = '\0';
    vm->output_len = 0;
    vm->output[0] = '\0';
    
    vm->lexer.source = source;
    vm->lexer.current = source;
    vm->lexer.line = 1;
    vm->lexer.col = 1;
    
    next(vm);
    
    while (!check(vm, TOK_EOF) && !vm->error) {
        parse_statement(vm);
        skip_newlines(vm);
    }
    
    return !vm->error;
}

const char *javi_get_output(javi_vm_t *vm) {
    return vm->output;
}

const char *javi_get_error(javi_vm_t *vm) {
    return vm->error ? vm->error_msg : NULL;
}

/* ========== STANDARD LIBRARY ========== */

static javi_value_t stdlib_abs(int argc, javi_value_t *argv) {
    if (argc > 0 && argv[0].type == VAL_NUMBER) {
        int32_t n = argv[0].number;
        return javi_number(n < 0 ? -n : n);
    }
    return javi_number(0);
}

static javi_value_t stdlib_min(int argc, javi_value_t *argv) {
    if (argc >= 2 && argv[0].type == VAL_NUMBER && argv[1].type == VAL_NUMBER) {
        return javi_number(argv[0].number < argv[1].number ? 
                          argv[0].number : argv[1].number);
    }
    return javi_number(0);
}

static javi_value_t stdlib_max(int argc, javi_value_t *argv) {
    if (argc >= 2 && argv[0].type == VAL_NUMBER && argv[1].type == VAL_NUMBER) {
        return javi_number(argv[0].number > argv[1].number ? 
                          argv[0].number : argv[1].number);
    }
    return javi_number(0);
}

static javi_value_t stdlib_len(int argc, javi_value_t *argv) {
    if (argc > 0 && argv[0].type == VAL_STRING) {
        return javi_number(strlen(argv[0].string));
    }
    return javi_number(0);
}

static javi_value_t stdlib_str(int argc, javi_value_t *argv) {
    if (argc > 0) {
        char buf[JAVI_MAX_STRING];
        value_to_string(&argv[0], buf, sizeof(buf));
        return javi_string(buf);
    }
    return javi_string("");
}

static javi_value_t stdlib_int(int argc, javi_value_t *argv) {
    if (argc > 0) {
        if (argv[0].type == VAL_NUMBER) return argv[0];
        if (argv[0].type == VAL_STRING) {
            return javi_number(atoi(argv[0].string));
        }
        if (argv[0].type == VAL_BOOL) {
            return javi_number(argv[0].boolean ? 1 : 0);
        }
    }
    return javi_number(0);
}

void javi_register_stdlib(javi_vm_t *vm) {
    javi_register_func(vm, "abs", stdlib_abs);
    javi_register_func(vm, "min", stdlib_min);
    javi_register_func(vm, "max", stdlib_max);
    javi_register_func(vm, "len", stdlib_len);
    javi_register_func(vm, "str", stdlib_str);
    javi_register_func(vm, "int", stdlib_int);
}
