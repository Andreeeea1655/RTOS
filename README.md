# ⚙️ RTOS — Real-Time Operating System (Bare-Metal ARM)

Un sistem de operare în timp real minimal implementat în C, rulând pe arhitectura **ARM Cortex-M3**, fără dependențe de biblioteci externe.

## Descriere

Proiectul implementează de la zero un RTOS bare-metal pentru microcontrolere ARM, incluzând gestionarea task-urilor, un scheduler și un startup personalizat. Totul este compilat cu toolchain-ul `arm-none-eabi-gcc` și linked cu un linker script propriu.

## Structura proiectului

```
RTOS/
├── src/
│   ├── startup.c     # Cod de inițializare (vector table, reset handler)
│   ├── main.c        # Punctul de intrare — definirea task-urilor
│   ├── rtos.c        # Kernel-ul RTOS (scheduler, context switching)
│   └── linker.ld     # Linker script pentru layout-ul memoriei
├── build/            # Fișiere compilate (.o, .elf)
└── Makefile
```

## Funcționalități

- **Startup bare-metal** — inițializare hardware fără OS sau runtime standard
- **Scheduler** — gestionarea și comutarea între task-uri
- **Context switching** — salvare/restaurare stare procesor
- **Linker script custom** — control complet asupra memoriei (flash, RAM, stack)

## Build

Necesită toolchain-ul ARM GCC instalat:

```bash
# Instalare toolchain (Linux)
sudo apt install gcc-arm-none-eabi

# Compilare
make

# Curățare
make clean
```

Output-ul se găsește în `build/rtos.elf`.

## Specificații tehnice

| Parametru | Valoare |
|---|---|
| Target | ARM Cortex-M3 |
| Toolchain | `arm-none-eabi-gcc` |
| ISA | Thumb (`-mthumb`) |
| Optimizare | `-O0 -g3` (debug) |
| Standard libraries | dezactivate (`-nostdlib`, `-nostartfiles`) |

## Tehnologii

- C (bare-metal, freestanding)
- ARM Cortex-M3 / Thumb ISA
- `arm-none-eabi-gcc`
- Linker script (`.ld`)
- Makefile
