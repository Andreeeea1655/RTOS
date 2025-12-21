#include "rtos.h"
#define SCB_SHPR3 (*(volatile uint32_t *)0xE000ED20) 
// Adresa SCB_ICSR (pentru PendSV trigger)
#define SCB_ICSR  (*(volatile uint32_t *)0xE000ED04)
#define SCB_ICSR_PENDSVSET (1UL << 28)
// ----------------------------------------------
// Pool static de TCB-uri si stive
// ----------------------------------------------
static rtos_tcb_t tcb_pool[RTOS_MAX_TASKS];
static uint32_t tcb_count = 0;
static uint32_t task_stacks[RTOS_MAX_TASKS][RTOS_STACK_SIZE] __attribute__((aligned(8)));
static rtos_tcb_t *current_task = NULL;
static rtos_tcb_t *ready_lists[RTOS_MAX_PRIORITIES];
static uint32_t top_priority_mask = 0;
static volatile uint32_t g_tick = 0;
static volatile uint32_t rtos_started=0;
extern void systick_init(void);
// ----------------------------------------------
// Functii pentru Tick
// ----------------------------------------------
void rtos_tick_handler(){
    g_tick++;
    // Parcurgem toate task-urile din pool-ul static
    for (uint32_t i = 0; i < tcb_count; i++) {
        if (tcb_pool[i].delay_ticks > 0) {
            tcb_pool[i].delay_ticks--;
        }
    }
    // Declanșăm PendSV pentru a verifica dacă un task proaspăt trezit are prioritate mai mare
    SCB_ICSR = SCB_ICSR_PENDSVSET;
}

uint32_t rtos_now(){
    return g_tick;
}

static uint32_t get_next_task_priority(uint32_t mask)
{
    if(mask==0) return 0;
    //calcul O(1) pentru gasirea celei mai inalte prioritati cu __buitlin_clz
    return 31 - __builtin_clz(mask);
}

static void set_exception_priorities()
{
    // Setează PendSV la 255 (cea mai mică) și SysTick la ceva mai mare (ex. 128)
    SCB_SHPR3 = (0xFF << 16) | (0x80 << 24);
}
// ----------------------------------------------
// Initializare RTOS
// ----------------------------------------------
void rtos_init(){
    tcb_count = 0;
    current_task = NULL;
    set_exception_priorities();
}
// ----------------------------------------------
// selecteaza urmatorul task de rulat
// ----------------------------------------------
void rtos_scheduler_next() {
    if(tcb_count == 0) return;

    uint32_t temp_mask = top_priority_mask;
    while(temp_mask > 0) {
        // Găsim cea mai înaltă prioritate din masca temporară
        uint32_t prio =get_next_task_priority(temp_mask);
        rtos_tcb_t *start_task = ready_lists[prio];
        rtos_tcb_t *search = start_task;

        do {
            if (search->delay_ticks == 0) {
                current_task = search;
                ready_lists[prio] = search; // Rotim lista pentru Round Robin
                return;
            }
            search = search->next;
        } while (search != start_task);

        // Dacă nu am găsit nimic la prioritatea asta, o eliminăm din mască și căutăm la următoarea
        temp_mask &= ~(1u << prio);
    }
}

// ----------------------------------------------
// PendSV_Handler pentru context switching
// ----------------------------------------------
__attribute__((naked))
void PendSV_Handler(void)
{
    __asm volatile(
        "MRS r0, PSP\n"               
        "CBZ r0, skip_save\n"           
        "STMDB r0!, {r4-r11}\n"       
        "LDR r1, =current_task\n"
        "LDR r1, [r1]\n"
        "STR r0, [r1]\n"
        
        "skip_save:\n"       
        "BL rtos_scheduler_next\n"
        
        "LDR r1, =current_task\n"
        "LDR r1, [r1]\n"
        "LDR r0, [r1]\n"             
        "LDMIA r0!, {r4-r11}\n"       
        "MSR PSP, r0\n"
        
        "LDR r0, =0xFFFFFFFD\n" 
        "MOV lr, r0\n"
        "BX lr\n"
    );
}


// ----------------------------------------------
// Creare task
// ----------------------------------------------
void rtos_task_create(void (*task_fn)(void), uint32_t priority){
    if(tcb_count >= RTOS_MAX_TASKS){
        return;
    }

    rtos_tcb_t *tcb = &tcb_pool[tcb_count];
    tcb->priority = priority;
    tcb->delay_ticks = 0;

    uint32_t *stack = task_stacks[tcb_count];
    uint32_t size = RTOS_STACK_SIZE;

    stack[size - 1] = 0x01000000;           // xPSR (thumb bit = 1)
    stack[size - 2] = (uint32_t)task_fn | 0x01;  // PC = functia task-ului
    stack[size - 3] = 0xFFFFFFFD;           // LR pentru thread mode cu PSP (stiva separata)
    stack[size - 4] = 0;                    // R12
    stack[size - 5] = 0;                    // R3
    stack[size - 6] = 0;                    // R2
    stack[size - 7] = 0;                    // R1
    stack[size - 8] = 0;                    // R0

    // context software (R4..R11) va fi salvat/restaurat ulterior
    tcb->stack_ptr = &stack[size - 16];

    //priority list management
    if (ready_lists[priority] == NULL) {
        tcb->next = tcb; // Primul task punctează la el însuși
        ready_lists[priority] = tcb;
    } else {
        // Inserăm task-ul nou în lista circulară existentă
        tcb->next = ready_lists[priority]->next;
        ready_lists[priority]->next = tcb;
    }
    
    top_priority_mask |= (1u << priority);
    tcb_count++;
}
// ----------------------------------------------
// Pornire scheduler
// ----------------------------------------------
void rtos_start(){
    rtos_scheduler_next(); // Alege primul task
    
    __asm volatile("mov r0, #0 \n msr psp, r0"); // Spune-i lui PendSV că e prima rulare
    
    set_exception_priorities();
    systick_init();
    
    __asm volatile("cpsie i" : : : "memory"); 

    SCB_ICSR = SCB_ICSR_PENDSVSET; // Declanșează PendSV
    while(1); 
}
// ----------------------------------------------
// Contect switch prin PendSV
// ----------------------------------------------
void rtos_yield() 
{
    SCB_ICSR = SCB_ICSR_PENDSVSET; //declansare PendSV
}

void rtos_delay(uint32_t ticks) {
    if (ticks == 0) return;

    __asm volatile("cpsid i" : : : "memory");
    current_task->delay_ticks = ticks;
    __asm volatile("cpsie i" : : : "memory");

    rtos_yield(); // Forțăm un context switch pentru a lăsa alt task să ruleze
}