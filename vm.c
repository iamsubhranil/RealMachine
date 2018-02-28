#include "vm.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

VirtualMachine* rm_new(){
    VirtualMachine *machine = (VirtualMachine *)malloc(sizeof(VirtualMachine));
    machine->memory = NULL;
    machine->PC = machine->SR = 0;
    for(uint8_t i = 0;i < 8;i++)
        machine->registers[i] = 0;
    return machine;
}

void rm_free(VirtualMachine *machine){
    free(machine->memory);
    free(machine);
}

bool rm_init(VirtualMachine *machine, uint32_t mem){
    uint8_t *m = (uint8_t *)malloc(sizeof(uint8_t) * mem);
    if(m == NULL)
        return false;
    machine->memory = m;
    machine->memSize = mem;
    return true;
}

bool rm_write_mem(VirtualMachine *machine, uint8_t *data, uint32_t size, uint32_t offset){
    if(offset > machine->memSize || (offset + size) > machine->memSize)
        return false;
    for(uint32_t i = 0;i < size;i++)
        machine->memory[i + offset] = data[i];
    return true;
}

#define REAL_COMPUTED_GOTO

//#define DEBUG_INSTRUCTIONS

void rm_run(VirtualMachine *machine, uint32_t offset){
    if(offset > machine->memSize)
        return;
    machine->PC = offset;

    static uint8_t instruction = 0;

    #define READ_BYTE(x) machine->memory[x]
    #define READ_WORD(x) ((READ_BYTE(x) << 8) | (READ_BYTE(x + 1)))
    #define READ_LONG(x) ((READ_WORD(x) << 16) | (READ_WORD(x + 2)))

    #define WRITE_BYTE(x, y) machine->memory[x] = y
    #define WRITE_WORD(x, y) WRITE_BYTE(x, (y & 0xff00) >> 8), WRITE_BYTE(x + 1, (y & 0xff))
    #define WRITE_LONG(x, y) WRITE_WORD(x, (y & 0xffff0000) >> 16), WRITE_WORD(x + 2, (y & 0xffff))

    #ifdef DEBUG_INSTRUCTIONS
    #define DEBUG_INS() { \
        uint32_t offs = machine->PC; \
        debugInstruction(machine->memory, &offs); \
        getc(stdin); }
    #else
    #define DEBUG_INS() {}
    #endif

    #ifdef REAL_COMPUTED_GOTO

    static void* dispatchTable[] = {
        #define OPCODE(name) &&code_##name,
        #include "opcodes.h"
        #undef OPCODE
    };

    #define CASE(name) code_##name
    #define DISPATCH() \
        DEBUG_INS(); \
        goto *dispatchTable[instruction = READ_BYTE(machine->PC)];
    #define INTERPRET_LOOP DISPATCH()

    #else

    #define CASE(name) case OP_##name
    #define DISPATCH() goto loop
    #define INTERPRET_LOOP \
        loop: \
        DEBUG_INS(); \
        switch(instruction = READ_BYTE(machine->PC))

    #endif

    #define INCR_PC(x) machine->PC += x

    #define BINARY(x) \
            regl(READ_BYTE(machine->PC + 2)) = regl(READ_BYTE(machine->PC + 1)) x \
                                regl(READ_BYTE(machine->PC + 2)); \
            INCR_PC(3); \
            DISPATCH()

    #define BICONDITIONAL(x) \
            if(regl(READ_BYTE(machine->PC + 1)) x regl(READ_BYTE(machine->PC + 2))){ \
                machine->PC = READ_LONG(machine->PC + 3); \
                DISPATCH(); \
            } \
            INCR_PC(7); \
            DISPATCH()

    #define STATUS_JUMP(x) \
            if(machine->SR == x) { \
                machine->PC = READ_BYTE(machine->PC + 1); \
                DISPATCH(); \
            } \
            INCR_PC(5); \
            DISPATCH();

    #define SHIFT(x) \
            regl(READ_BYTE(machine->PC + 1)) = regl(READ_BYTE(machine->PC + 1)) x READ_LONG(machine->PC + 2); \
            INCR_PC(6); \
            DISPATCH();

    INTERPRET_LOOP
    {
        CASE(ADD):
            BINARY(+);
        CASE(SUB):
            BINARY(-);
        CASE(MUL):
            BINARY(*);
        CASE(DIV):
            BINARY(/);
        CASE(AND):
            BINARY(&);
        CASE(OR):
            BINARY(|);
        CASE(NOT):
            regl(READ_BYTE(machine->PC + 1)) = ~regl(READ_BYTE(machine->PC + 1));
            INCR_PC(2);
            DISPATCH();
        CASE(LSHIFT):
            SHIFT(<<);
        CASE(RSHIFT):
            SHIFT(>>);
        CASE(LOAD):
            regl(READ_BYTE(machine->PC + 5)) = READ_LONG(READ_LONG(machine->PC + 1));
            INCR_PC(6);
            DISPATCH();
        CASE(STORE):
            WRITE_LONG(READ_LONG(machine->PC + 2), regl(READ_BYTE(machine->PC + 1)));
            INCR_PC(6);
            DISPATCH();
        CASE(MOV):
            regl(READ_BYTE(machine->PC + 5)) = READ_LONG(machine->PC + 1);
            INCR_PC(6);
            DISPATCH();
        CASE(SAVE):
            WRITE_LONG(READ_LONG(machine->PC + 5), READ_LONG(machine->PC + 1));
            INCR_PC(9);
            DISPATCH();
        CASE(PRINT):
            printf("%" PRIu32, (uint32_t)READ_LONG(READ_LONG(machine->PC + 1)));
            INCR_PC(5);
            DISPATCH();
        CASE(PRINTC):
            printf("%c", READ_LONG(READ_LONG(machine->PC + 1)));
            INCR_PC(5);
            DISPATCH();
        CASE(JEQ):
            BICONDITIONAL(==);
        CASE(JNE):
            BICONDITIONAL(!=);
        CASE(JGT):
            BICONDITIONAL(>);
        CASE(JLT):
            BICONDITIONAL(<);
        CASE(JOV):
            STATUS_JUMP(1);
        CASE(JUN):
            STATUS_JUMP(2);
        CASE(CLRPC):
            machine->PC = 0;
            DISPATCH();
        CASE(CLRSR):
            machine->SR = 0;
            INCR_PC(1);
            DISPATCH();
        CASE(HALT):
            return;
    }
}
