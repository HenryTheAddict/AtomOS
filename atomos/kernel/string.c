/*
 * AtomOS - String Functions
 * Basic string and memory operations for the kernel
 */

#include "include/types.h"

/*
 * Set memory to a value
 */
void *memset(void *dest, int value, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    while (count--) {
        *d++ = (uint8_t)value;
    }
    return dest;
}

/*
 * Copy memory
 */
void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

/*
 * Move memory (handles overlapping regions)
 */
void *memmove(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    
    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    
    return dest;
}

/*
 * Compare memory
 */
int memcmp(const void *s1, const void *s2, size_t count) {
    const uint8_t *a = (const uint8_t *)s1;
    const uint8_t *b = (const uint8_t *)s2;
    
    while (count--) {
        if (*a != *b) {
            return *a - *b;
        }
        a++;
        b++;
    }
    
    return 0;
}

/*
 * Get string length
 */
size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

/*
 * Copy string
 */
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/*
 * Copy string with limit
 */
char *strncpy(char *dest, const char *src, size_t count) {
    char *d = dest;
    
    while (count && (*d++ = *src++)) {
        count--;
    }
    
    while (count--) {
        *d++ = '\0';
    }
    
    return dest;
}

/*
 * Compare strings
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t *)s1 - *(const uint8_t *)s2;
}

/*
 * Compare strings with limit
 */
int strncmp(const char *s1, const char *s2, size_t count) {
    while (count && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        count--;
    }
    
    if (count == 0) {
        return 0;
    }
    
    return *(const uint8_t *)s1 - *(const uint8_t *)s2;
}

/*
 * Concatenate strings
 */
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

/*
 * Find character in string
 */
char *strchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return (c == '\0') ? (char *)str : NULL;
}

/*
 * Find last occurrence of character in string
 */
char *strrchr(const char *str, int c) {
    const char *last = NULL;
    while (*str) {
        if (*str == (char)c) {
            last = str;
        }
        str++;
    }
    return (c == '\0') ? (char *)str : (char *)last;
}

/*
 * Find substring
 */
char *strstr(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    
    if (needle_len == 0) {
        return (char *)haystack;
    }
    
    while (*haystack) {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    
    return NULL;
}

/*
 * Duplicate string (requires kmalloc)
 */
extern void *kmalloc(size_t size);

char *strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *new_str = (char *)kmalloc(len);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}

/*
 * Convert string to integer
 */
int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

/*
 * Convert integer to string
 */
char *itoa(int value, char *str, int base) {
    char *p = str;
    char *p1, *p2;
    unsigned int uvalue;
    char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    if (value < 0 && base == 10) {
        *p++ = '-';
        uvalue = -value;
    } else {
        uvalue = (unsigned int)value;
    }
    
    char *start = p;
    do {
        *p++ = digits[uvalue % base];
        uvalue /= base;
    } while (uvalue);
    
    *p = '\0';
    
    /* Reverse */
    p1 = start;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
    
    return str;
}

/*
 * Check if character is whitespace
 */
int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

/*
 * Check if character is digit
 */
int isdigit(int c) {
    return c >= '0' && c <= '9';
}

/*
 * Check if character is alphabetic
 */
int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/*
 * Check if character is alphanumeric
 */
int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

/*
 * Convert to uppercase
 */
int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

/*
 * Convert to lowercase
 */
int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}
