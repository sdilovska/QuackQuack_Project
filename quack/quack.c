/*
QuackQuack — Build a Tiny Computer in C
=================================

You only edit THIS file: quack.c

The labs will teach you C while you build this tiny computer.

---------------------------------------------------------------------------
How to read this file
---------------------------------------------------------------------------
This file is long, but it is written in a friendly order (your lecturers are very nice, you are welcome).

1) CONSTANTS
   - memory size and special I/O addresses
   - instruction opcodes (machine language numbers)

2) MEMORY (Lab 1)
   - read/write bytes
   - read/write 16-bit words (little-endian)

3) I/O DEVICES (Lab 4)
   - keyboard input
   - screen output

4) CPU (Labs 2–5)
   - CPU state (registers, PC, SP, ZF)
   - fetch → decode → execute loop (cpu_step)

5) LOADER + CLI (provided)
   - loads .duck programs into memory
   - runs them

---------------------------------------------------------------------------
Mini C crash course (just enough to start)
---------------------------------------------------------------------------
C is a compiled language:
- You write .c code
- You compile it into an executable program (here: ./quack)

We use these types:
- uint8_t   : an unsigned 8-bit number (0..255)  (a byte)
- uint16_t  : an unsigned 16-bit number (0..65535)
- int       : normal integer (used for counters/loops)

Arrays:
- memory is an array of 4096 bytes:
    memory[0] ... memory[4095]
- Accessing outside the array is a bug → we prevent this with bounds checks.

Functions:
- A function is a named block of code you can call.
- Example:
    uint8_t mem_read8(uint16_t addr) { ... }

Structs:
- A struct is a bundle of variables.
- We use structs for CPU state and instructions.

---------------------------------------------------------------------------
Instruction format (important)
---------------------------------------------------------------------------
Each instruction is exactly 4 bytes in memory:

  byte0: opcode  (what to do)
  byte1: ra      (register number 0..3)
  byte2: b2      (meaning depends on instruction)
  byte3: b3      (meaning depends on instruction)

Sometimes b2 and b3 form a 16-bit value (little-endian):
  imm16 = b2 | (b3 << 8)

Little-endian means the LOW byte comes first in memory.

---------------------------------------------------------------------------
Programs
---------------------------------------------------------------------------
In programs/ you will find compiled programs (binary) ending in .duck.
You do not change them during the labs.

- sum10.duck     : sums 1..10  (tests CPU basics)
- call_add.duck  : tests CALL/RET and stack
- maze.duck      : a maze game using I/O and byte loads/stores

---------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
READING ROADMAP
--------------
Skim these sections first:
- Constants (memory map + opcodes)
- Memory functions (Lab 1)
- cpu_step (Labs 2–4)

You will not write everything at once.
The labs will guide you.
*/

/* =========================
   Constants (memory map)
   ========================= */
#define MEM_SIZE 4096

#define CODE_START  0x0000
#define CODE_END    0x07FF
#define DATA_START  0x0800
#define DATA_END    0x0BDF
#define IO_START    0x0BE0
#define IO_END      0x0BFF
#define STACK_START 0x0C00
#define STACK_END   0x0FFF

#define SP_INIT     0x1000   /* stack pointer starts one past 0x0FFF */

/* I/O registers (memory-mapped) */
#define IO_KEY     0x0BE0  /* read: ASCII key; consumes it */
#define IO_STATUS  0x0BE1  /* read: 1 if key available else 0 */
#define IO_PUTCHAR 0x0BE2  /* write: prints one character */
#define IO_CLEAR   0x0BE3  /* write: clears the terminal */
#define IO_TICK    0x0BE4  /* read: low 8 bits of tick counter */

/* =========================
   Instruction opcodes
   =========================

These numbers are the machine language.
A .duck program is just a sequence of bytes containing these opcodes.
In Labs 2–4 you implement them inside cpu_step using switch-case.

*/
/* Data movement */
#define OP_RRMOVW  0x01  /* rrmovw  ra <- rb          (rb in b2) */
#define OP_IRMOVB  0x02  /* irmovb  ra <- imm8        (imm8 in b3) */
#define OP_MRMOVW  0x03  /* mrmovw  ra <- mem16[imm16] */
#define OP_RMMOVW  0x04  /* rmmovw  mem16[imm16] <- ra */
#define OP_IRMOVW  0x05  /* irmovw  ra <- imm16       (imm16 in b2,b3) */

/* Byte load/store (used by maze) */
#define OP_MRMOVB  0x06  /* mrmovb  ra <- mem8[imm16]  */
#define OP_RMMOVB  0x07  /* rmmovb  mem8[imm16] <- low8(ra) */
#define OP_MRMOVBR 0x08  /* mrmovbR ra <- mem8[Rb]     (rb in b2) */
#define OP_RMMOVBR 0x09  /* rmmovbR mem8[Rb] <- low8(ra) */

/* ALU */
#define OP_ADDW    0x10
#define OP_SUBW    0x11
#define OP_CMPW    0x15  /* sets ZF */

/* Control flow */
#define OP_JMP     0x20
#define OP_JE      0x21  /* jump if ZF==1 */
#define OP_JNE     0x22  /* jump if ZF==0 */
#define OP_HALT    0x23

/* Stack / procedures */
#define OP_PUSHW   0x30
#define OP_POPW    0x31
#define OP_CALL    0x32
#define OP_RET     0x33

/* =========================
   Memory (Lab 1)
   ========================= */
static uint8_t memory[MEM_SIZE];

/* Print an error and stop the program */
/* Print a message and stop the program.
   We use this for unrecoverable errors.
   Note: printf prints to the terminal.
*/
static void die(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}

/* Called when a program tries to read/write outside memory.
   This is like a "segmentation fault" in a real machine.
*/
static void memory_error(uint16_t addr) {
    printf("MEMORY ERROR at 0x%04X\n", addr);
    exit(1);
}

/* TODO (Lab 1): implement bounds-checked byte read */
static uint8_t mem_read8(uint16_t addr) {
    (void)addr;

    if(addr >= MEM_SIZE){
        memory_error(addr);
    }
    else{
        return memory[addr];
    }
    return 0;
}

/* TODO (Lab 1): implement bounds-checked byte write */
static void mem_write8(uint16_t addr, uint8_t v) {
    (void)addr; (void)v;
    
    if(addr >= MEM_SIZE){
        memory_error(addr);
    }
    else{
        memory[addr] = v;
    }
}

/* TODO (Lab 1): implement bounds-checked 16-bit little-endian read */
static uint16_t mem_read16(uint16_t addr) {
    (void)addr;
    if((addr + 1) >= MEM_SIZE ) {
        memory_error(addr);
    }
    else {
        u_int8_t low = memory[addr];
        u_int8_t high = memory[addr+1];
        u_int16_t result = low | (high << 8);
        return result;
    }
    return 0;
}

/* TODO (Lab 1): implement bounds-checked 16-bit little-endian write */
static void mem_write16(uint16_t addr, uint16_t v) {
    (void)addr; (void)v;
    if(addr >= MEM_SIZE || (addr+1) >= MEM_SIZE){
        memory_error(addr);
    }
    else {
        memory[addr] = v & 0xFF;
        memory[addr+1] = (v >> 8) & 0xFF;
    }
}

/* =========================
   I/O device (Lab 4)
   ========================= */
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/select.h>
#define POSIX_INPUT 1
#else
#define POSIX_INPUT 0
#endif

static unsigned int tick_counter = 0;
static const char *script_input = NULL;
static int script_pos = 0;

static uint8_t last_key = 0;
static int key_available = 0;

static uint8_t to_upper(uint8_t c) {
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a' + 'A');
    return c;
}

static void io_init(void) {
    tick_counter = 0;
    script_input = NULL;
    script_pos = 0;
    last_key = 0;
    key_available = 0;
}

static void io_set_script(const char *s) {
    script_input = s;
    script_pos = 0;
    last_key = 0;
    key_available = 0;
}

/* Called once per CPU step */
static void io_tick(void) {
    tick_counter++;

    /* Scripted input first (deterministic) */
    if (script_input != NULL && !key_available) {
        char c = script_input[script_pos];
        if (c != '\0') {
            last_key = to_upper((uint8_t)c);
            key_available = 1;
            script_pos++;
            return;
        }
    }

#if POSIX_INPUT
    /* Non-blocking live input: only read if stdin is ready */
    if (!key_available) {
        fd_set set;
        struct timeval tv;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &tv);
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &set)) {
            unsigned char c = 0;
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n == 1) {
                if (c == '\n' || c == '\r') return;
                last_key = to_upper((uint8_t)c);
                key_available = 1;
            }
        }
    }
#else
    /* Non-POSIX systems: use --script for input. */
    (void)0;
#endif
}

/* TODO (Lab 4): implement memory-mapped input reads */
static uint8_t io_read8(uint16_t addr) {
    (void)addr;
    return 0;
}

/* TODO (Lab 4): implement memory-mapped output writes */
static void io_write8(uint16_t addr, uint8_t v) {
    (void)addr; (void)v;
}

/* =========================
   CPU (Labs 2–5)
   ========================= */
/* CPU state (registers + special registers).
   - pc: program counter
   - sp: stack pointer
   - r[0..3]: general registers
   - zf: zero flag (used by JE/JNE)
*/
typedef struct {
    uint16_t pc;      /* program counter (address of next instruction) */
    uint16_t sp;      /* stack pointer */
    uint16_t r[4];    /* R0..R3 */
    uint8_t zf;       /* zero flag */
    int halted;       /* 1 if HALT executed */
} cpu_t;

/* One decoded instruction (4 bytes). */
typedef struct {
    uint8_t op;
    uint8_t ra;
    uint8_t b2;
    uint8_t b3;
} instr_t;

static uint16_t u16_from_le(uint8_t lo, uint8_t hi) {
    return (uint16_t)(lo | ((uint16_t)hi << 8));
}

static void cpu_reset(cpu_t *cpu) {
    cpu->pc = 0;
    cpu->sp = SP_INIT;
    cpu->r[0] = cpu->r[1] = cpu->r[2] = cpu->r[3] = 0;
    cpu->zf = 0;
    cpu->halted = 0;
}

static void check_reg(uint8_t r) {
    if (r > 3) die("Invalid register index (valid: 0..3)");
}

static instr_t fetch(cpu_t *cpu) {
    instr_t in;
    in.op = mem_read8(cpu->pc + 0);
    in.ra = mem_read8(cpu->pc + 1);
    in.b2 = mem_read8(cpu->pc + 2);
    in.b3 = mem_read8(cpu->pc + 3);
    return in;
}

/* CPU STEP (the heart of the project)
   ---------------------------------
   cpu_step executes ONE instruction.

   It does:
   1) io_tick()        (updates input and tick counter)
   2) fetch()          (reads 4 bytes at pc)
   3) switch(opcode)   (executes the instruction)

   Most instructions increment pc by 4.
   Jumps/CALL/RET set pc to a new value.
*/
/* TODO (Labs 2–5): implement CPU instruction execution */
static void cpu_step(cpu_t *cpu, int debug) {
    (void)debug;

    if (cpu->halted) return;

    io_tick();
    instr_t in = fetch(cpu);

    /* In debug mode, print a simple trace */
    if (debug) {
        printf("PC=%04X OP=%02X R0=%04X R1=%04X R2=%04X R3=%04X ZF=%u SP=%04X\n",
               cpu->pc, in.op, cpu->r[0], cpu->r[1], cpu->r[2], cpu->r[3], cpu->zf, cpu->sp);
    }

    /* You will implement this switch gradually:
       Lab 2: irmovw, addw, cmpw, je/jmp, halt
       Lab 3: pushw, popw, call, ret
       Lab 4: byte loads/stores + I/O reads/writes
       Lab 5: polish + optional stats
    */
    //lab 2
    /*case OP_ADDW:
    check_reg(ra);
    check_reg(rb);
    r[rb] = r[rb] + r[ra];
    zf = (r[rb] == 0);
    pc += 4;
    break;*/
    switch(in.op){
        case OP_IRMOVW: {
            check_reg(in.ra);
            uint16_t imm16 = u16_from_le(in.b2, in.b3);
            cpu->r[in.ra] = imm16;
            cpu->zf = (cpu->r[in.ra] == 0);
            cpu->pc+=4;
            break;
        }
        case OP_RRMOVW: {
            check_reg(in.ra);
            check_reg(in.b2);
            cpu->r[in.b2] = cpu->r[in.ra];
            cpu->zf = (cpu->r[in.b2] == 0);
            cpu->pc+=4;
            break;
        }
        case OP_IRMOVB: {
            check_reg(in.ra);
            uint8_t imm8 = in.b3;
            cpu->r[in.ra] = imm8;
            cpu->zf = (cpu->r[in.ra] == 0);
            cpu->pc+=4;
            break;
        }
        case OP_ADDW: {
            check_reg(in.ra);
            check_reg(in.b2);
            cpu->r[in.b2] = (cpu->r[in.b2] + cpu->r[in.ra]);
            cpu->zf = (cpu->r[in.b2] == 0);
            cpu->pc+=4;
            break;
        }
        case OP_SUBW: {
            check_reg(in.ra);
            check_reg(in.b2);
            cpu->r[in.b2] = (cpu->r[in.b2] - cpu->r[in.ra]);
            cpu->zf = (cpu->r[in.b2] == 0);
            cpu->pc+=4;
            break;
        }
        case OP_CMPW: {
            if(cpu->r[in.ra] == cpu->r[in.b2]){
                cpu->zf = 1;
            }
            else{
                cpu->zf = 0;
            }
            cpu->pc+=4;
            break;
        }
        case OP_JMP: {
            cpu->pc = u16_from_le(in.b2, in.b3);
            break;
        }
        case OP_JE: {
            if(cpu->zf == 1){
                cpu->pc = u16_from_le(in.b2, in.b3);
            }
            else{
                cpu->pc += 4;
            }
            break;
        }
        case OP_JNE: {
            if(cpu->zf == 0){
                cpu->pc = u16_from_le(in.b2, in.b3);
            }
            else{
                cpu->pc += 4;
            }
            break;
        }
        case OP_HALT: {
            cpu->halted = 1;
            break;
        }

        case OP_POPW: {
            cpu->r[in.ra] = mem_read16(cpu->sp);
            cpu->sp = cpu->sp + 2;
            cpu->zf = 0;
            cpu->pc += 4;
        }

        case OP_RET: {
            cpu->pc = mem_read16(cpu->sp);
            cpu->sp = cpu->sp + 2;
        }

        case OP_PUSHW: {
            cpu->sp = cpu->sp - 2;
            mem_write16(cpu->sp, cpu->r[in.ra]);
            cpu->pc += 4;
        }
    }
    // die("cpu_step not implemented yet. Start with docs/LAB2.md");
}

static void cpu_run(cpu_t *cpu, int debug) {
    while (!cpu->halted) {
        cpu_step(cpu, debug);
        if (cpu->pc >= MEM_SIZE && !cpu->halted) die("PC out of bounds");
    }
}

/* =========================
   Program loader (provided)
   ========================= */
static void mem_clear(void) {
    for (int i = 0; i < MEM_SIZE; i++) memory[i] = 0;
}

static void load_program(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("Could not open program file");

    int c;
    int addr = 0;
    while ((c = fgetc(f)) != EOF) {
        if (addr >= MEM_SIZE) die("Program too large for memory");
        memory[addr++] = (uint8_t)c;
    }
    fclose(f);
}

/* =========================
   CLI (provided)
   ========================= */
static void usage(void) {
    printf("Usage:\n");
    printf("  ./quack run [--debug] [--script \"KEYS\"] <program.duck>\n");
}

int main(int argc, char **argv) {
    if (argc < 3) { usage(); return 1; }
    if (strcmp(argv[1], "run") != 0) { usage(); return 1; }

    int debug = 0;
    const char *script = NULL;
    const char *path = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) debug = 1;
        else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) script = argv[++i];
        else path = argv[i];
    }

    if (!path) { usage(); return 1; }

    mem_clear();
    io_init();
    if (script) io_set_script(script);

    if (!script && strstr(path, "maze") != NULL) {
        printf("Maze input: type W/A/S/D/Q (or lowercase) and press Enter. Or use --script.\n");
    }

    load_program(path);

    cpu_t cpu;
    cpu_reset(&cpu);

    cpu_run(&cpu, debug);

    printf("\nHALT\n");
    printf("R0(final)=%04X\n", cpu.r[0]);
    return 0;
}
