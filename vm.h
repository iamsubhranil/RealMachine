#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef union{
    uint8_t byte[4];
    uint16_t word[2];
    uint32_t lng;
} Register;

typedef enum{
    #define OPCODE(name) OP_##name,
    #include "opcodes.h"
    #undef OPCODE
} Code;

typedef struct{
    uint32_t registers[8];
    uint32_t PC;
    uint8_t SR;
    uint8_t *memory;
    uint32_t memSize;
} VirtualMachine;

VirtualMachine* rm_new();
bool rm_init(VirtualMachine *machine, uint32_t memSize);
bool rm_write_mem(VirtualMachine *machine, uint8_t *data, uint32_t size, uint32_t offset);
void rm_run(VirtualMachine *machine, uint32_t offset);
void rm_free(VirtualMachine *machine);

#define regl(index) machine->registers[index]
