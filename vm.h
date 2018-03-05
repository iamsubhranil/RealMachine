#pragma once

#include <stdint.h>
#include <stdbool.h>

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
    uint64_t PC;
    uint8_t *memory;
} VirtualMachine;

VirtualMachine* rm_new();
bool rm_init(VirtualMachine *machine, uint32_t memSize);
void rm_run(VirtualMachine *machine, uint32_t offset);
void rm_free(VirtualMachine *machine);

#define regl(index) machine->registers[index]
