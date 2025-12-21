#ifndef RTOS_H
#define RTOS_H

#include <stdint.h>
#include <stddef.h>
#include "rtos_config.h"

// ----------------------------------------------
// Task Control Block
// ----------------------------------------------
typedef struct rtos_tcb{
    uint32_t *stack_ptr;    // pointer la varful stivei taskului
    uint32_t priority;      // prioritatea
    uint32_t delay_ticks;   // delay in ticks
    struct rtos_tcb *next;  // pt ready lists
}rtos_tcb_t;

// ----------------------------------------------
// definitii Hardware (registrii SCB pentru PendSV)
// ----------------------------------------------
#define SCB_ICSR   (*(volatile uint32_t *)0xE000ED04) // Interrupt Control and State Register
#define SCB_ICSR_PENDSVSET (1UL << 28)                // bit-ul pentru a declansa PendSV

// ----------------------------------------------
// API
// ----------------------------------------------
void rtos_init();
void rtos_task_create(void (*task_fn)(void), uint32_t priority);
void rtos_scheduler_next(void);                       
void rtos_start();
void rtos_delay(uint32_t ticks);
void rtos_tick_handler();
uint32_t rtos_now();
void rtos_yield();   //forteaza switch ul

#endif