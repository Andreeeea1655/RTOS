#include "rtos.h"

extern uint32_t rtos_now();

// ----------------------------------------------
// SysTick
// ----------------------------------------------
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010) // Control and Status
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014) // Reload Value
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018) // Current Value

#define SCB_VTOR   (*(volatile uint32_t *)0xE000ED08)

#define SYST_CSR_ENABLE      (1u << 0)  // enable counter
#define SYST_CSR_TICKINT     (1u << 1)  // enable interrupt
#define SYST_CSR_CLKSOURCE   (1u << 2)  // 1 = core clock

//task uri de test
rtos_queue_t q_date;
volatile uint32_t msj_trimise = 0;
volatile uint32_t msj_primite = 0;
volatile uint32_t ultimul_mesaj = 0;

void systick_init()
{
    uint32_t reload = (CPU_CLOCK_HZ / RTOS_TICK_RATE_HZ) - 1u;
    if(reload > 0x00FFFFFFu){
        while(1){}
    }

    SYST_RVR = reload;        // setam valoarea de reload
    SYST_CVR = 0u;            // resetam contorul curent
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}
// ----------------------------------------------
// Task-uri
// ----------------------------------------------
void task_producator(void) {
    uint32_t count=100;
    while(1) {
        rtos_delay(1000); //trimite un mesaj pe secunda
        rtos_queue_send(&q_date, count);
        msj_trimise++;
        count++;
    }
}
void task_consumator(void) {
    while(1) {
    // Acest task va sta BLOCKED până când producătorul trimite ceva
        ultimul_mesaj = rtos_queue_receive(&q_date);
        msj_primite++;
    }
}
void idle_task(void) {
    while(1) {
        // Nu face nimic, doar previne blocarea scheduler-ului
    }
}
// ----------------------------------------------
// Main
// ----------------------------------------------
int main(){

    SCB_VTOR = 0x08000000;

    rtos_init();

    rtos_queue_init(&q_date);

    /*rtos_task_create(idle_task, 0); // Task-ul idle cu prioritate minima 0
    rtos_task_create(task_low, 1);
    rtos_task_create(task_med, 2);
    rtos_task_create(task_high, 3);*/

    rtos_task_create(idle_task, 0); //prio minima
    rtos_task_create(task_producator, 1); //prio medie
    rtos_task_create(task_consumator, 2); //prio mare - va procesa mesajul imd

    rtos_start();

    while(1){}
}