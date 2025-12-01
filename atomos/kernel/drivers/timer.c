/*
 * AtomOS - PIT Timer Implementation
 */

#include "timer.h"
#include "../include/kernel.h"
#include "../arch/x86/idt.h"

/* Timer state */
static volatile uint64_t timer_ticks = 0;
static timer_callback_t timer_cb = NULL;

/*
 * Timer interrupt handler
 */
static void timer_handler(registers_t *regs) {
    UNUSED registers_t *r = regs;
    timer_ticks++;
    
    if (timer_cb) {
        timer_cb(timer_ticks);
    }
}

/*
 * Initialize the PIT timer
 */
void timer_init(void) {
    /* Calculate divisor for desired frequency */
    uint32_t divisor = PIT_BASE_FREQ / TIMER_HZ;
    
    /* Send command byte: Channel 0, lo/hi byte, rate generator */
    outb(PIT_CMD, 0x36);
    
    /* Send divisor */
    outb(PIT_CH0_DATA, divisor & 0xFF);
    outb(PIT_CH0_DATA, (divisor >> 8) & 0xFF);
    
    /* Register interrupt handler */
    register_interrupt_handler(IRQ0, timer_handler);
    
    kprintf("Timer initialized at %d Hz\n", TIMER_HZ);
}

/*
 * Set timer callback
 */
void timer_set_callback(timer_callback_t callback) {
    timer_cb = callback;
}

/*
 * Get current tick count
 */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/*
 * Get milliseconds since boot
 */
uint64_t timer_get_ms(void) {
    return (timer_ticks * 1000) / TIMER_HZ;
}

/*
 * Get uptime in seconds
 */
uint64_t timer_get_uptime(void) {
    return timer_ticks / TIMER_HZ;
}

/*
 * Sleep for specified ticks
 */
void timer_sleep_ticks(uint32_t ticks) {
    uint64_t end = timer_ticks + ticks;
    while (timer_ticks < end) {
        hlt();
    }
}

/*
 * Sleep for milliseconds
 */
void timer_sleep_ms(uint32_t ms) {
    uint32_t ticks = (ms * TIMER_HZ) / 1000;
    if (ticks == 0) ticks = 1;
    timer_sleep_ticks(ticks);
}

/*
 * Sleep for seconds
 */
void timer_sleep_sec(uint32_t sec) {
    timer_sleep_ticks(sec * TIMER_HZ);
}

/*
 * Busy wait for microseconds (less accurate, but doesn't require interrupts)
 */
void timer_busy_wait_us(uint32_t us) {
    /* Use PIT channel 2 for timing */
    uint32_t count = (us * PIT_BASE_FREQ) / 1000000;
    if (count < 1) count = 1;
    
    /* Program PIT channel 2 for one-shot mode */
    outb(PIT_CMD, 0xB0);
    outb(PIT_CH2_DATA, count & 0xFF);
    outb(PIT_CH2_DATA, (count >> 8) & 0xFF);
    
    /* Wait for count to complete */
    while (!(inb(0x61) & 0x20));
}
