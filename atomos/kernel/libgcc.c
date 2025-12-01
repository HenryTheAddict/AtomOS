/*
 * AtomOS - GCC Helper Functions
 * Provides 64-bit arithmetic for 32-bit systems
 */

#include "include/types.h"

/*
 * 64-bit unsigned division
 */
uint64_t __udivdi3(uint64_t num, uint64_t den) {
    uint64_t quot = 0;
    uint64_t bit = 1;
    
    if (den == 0) {
        return 0;  /* Division by zero */
    }
    
    /* Align divisor to highest bit of numerator */
    while (den <= num && !(den & (1ULL << 63))) {
        den <<= 1;
        bit <<= 1;
    }
    
    /* Perform division */
    while (bit) {
        if (num >= den) {
            num -= den;
            quot |= bit;
        }
        den >>= 1;
        bit >>= 1;
    }
    
    return quot;
}

/*
 * 64-bit unsigned modulo
 */
uint64_t __umoddi3(uint64_t num, uint64_t den) {
    uint64_t quot = __udivdi3(num, den);
    return num - (quot * den);
}

/*
 * 64-bit signed division
 */
int64_t __divdi3(int64_t num, int64_t den) {
    int negative = 0;
    
    if (num < 0) {
        num = -num;
        negative = !negative;
    }
    
    if (den < 0) {
        den = -den;
        negative = !negative;
    }
    
    uint64_t result = __udivdi3((uint64_t)num, (uint64_t)den);
    
    return negative ? -(int64_t)result : (int64_t)result;
}

/*
 * 64-bit signed modulo
 */
int64_t __moddi3(int64_t num, int64_t den) {
    int negative = 0;
    
    if (num < 0) {
        num = -num;
        negative = 1;
    }
    
    if (den < 0) {
        den = -den;
    }
    
    uint64_t result = __umoddi3((uint64_t)num, (uint64_t)den);
    
    return negative ? -(int64_t)result : (int64_t)result;
}
