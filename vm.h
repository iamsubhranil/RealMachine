#pragma once

#include <stdint.h>
#include <stdbool.h>

// This macro sanitizes all memory access, i.e.
// read and writes, by checking whether they are
// out of bounds at each access explicitly.
// Needless to say, this slows down the machine
// to a large degree. Hence, it is defined here
// as a switch between performance and security.
//
// #define SANITIZE_ACCESS

typedef union{
    uint8_t byte[4];
    uint16_t word[2];
    uint32_t lng;
} Register;

typedef enum{
    #define OPCODE(name, a, b) OP_##name,
    #include "opcodes.h"
    #undef OPCODE
} Code;

typedef struct{
    uint8_t SR;
    uint32_t memSize;
    uint32_t registers[8];
#ifdef SANITIZE_ACCESS
    uint32_t AR; // Access register, to store the address of fault access
#endif
    uint64_t PC;
    uint8_t *memory;
} VirtualMachine;

VirtualMachine* rm_new();
bool rm_init(VirtualMachine *machine, uint32_t memSize);
void rm_run(VirtualMachine *machine, uint32_t offset);
void rm_free(VirtualMachine *machine);

#define regl(index) machine->registers[index]
