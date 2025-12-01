/*
 * AtomOS - PIT Timer Driver
 * Programmable Interval Timer for system timing
 */

#ifndef _ATOMOS_TIMER_H
#define _ATOMOS_TIMER_H

#include "../include/types.h"

/* PIT ports */
#define PIT_CH0_DATA    0x40
#define PIT_CH1_DATA    0x41
#define PIT_CH2_DATA    0x42
#define PIT_CMD         0x43

/* PIT frequency */
#define PIT_BASE_FREQ   1193182
#define TIMER_HZ        100     /* 100 Hz = 10ms intervals */

/* Timer callback */
typedef void (*timer_callback_t)(uint64_t ticks);

/* Function declarations */
void timer_init(void);
void timer_set_callback(timer_callback_t callback);

/* Time functions */
uint64_t timer_get_ticks(void);
uint64_t timer_get_ms(void);
uint64_t timer_get_uptime(void);

/* Delays */
void timer_sleep_ticks(uint32_t ticks);
void timer_sleep_ms(uint32_t ms);
void timer_sleep_sec(uint32_t sec);

/* High precision timing */
void timer_busy_wait_us(uint32_t us);

#endif /* _ATOMOS_TIMER_H */
