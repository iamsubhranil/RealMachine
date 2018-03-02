#include "vm.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static uint32_t instructionLength[] = {
    #define OPCODE(a, length, b) length,
    #include "opcodes.h"
    #undef OPCODE
};

void write_op(uint8_t *memory, uint32_t *offset, int opcode, ...){
    if(opcode == OP_const)
        (*offset)--;
    else
        memory[*offset] = opcode;

#define WRITE_LONG(o, val) \
    memory[o + 1] = val >> 24; \
    memory[o + 2] = val >> 16; \
    memory[o + 3] = val >> 8; \
    memory[o + 4] = val;

    va_list args;
    va_start(args, opcode);
    switch(opcode){
        case OP_add:
        case OP_sub:
        case OP_mul:
        case OP_div:
        case OP_and:
        case OP_or:
            memory[*offset + 1] = va_arg(args, int);
            memory[*offset + 2] = va_arg(args, int);
            break;
        case OP_not:
            memory[*offset + 1] = va_arg(args, int);
            break;
        case OP_lshift:
        case OP_rshift:{
                           memory[*offset + 1] = va_arg(args, int);
                           uint32_t val = va_arg(args, uint32_t);
                           WRITE_LONG(*offset + 1, val);
                           break;
                       }
        case OP_load:{
                         uint32_t val = va_arg(args, uint32_t);
                         WRITE_LONG(*offset, val);
                         memory[*offset + 5] = va_arg(args, int);
                         break;
                     }
        case OP_store:{
                          memory[*offset + 1] = va_arg(args, int);
                          uint32_t val = va_arg(args, uint32_t);
                          WRITE_LONG(*offset + 1, val);
                          break;
                      }
        case OP_mov:{
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        memory[*offset + 5] = va_arg(args, int);
                        break;
                    }
        case OP_save:{
                         uint32_t val1 = va_arg(args, uint32_t);
                         uint32_t val2 = va_arg(args, uint32_t);
                         WRITE_LONG(*offset, val1);
                         WRITE_LONG(*offset + 4, val2);
                         break;
                     }
        case OP_print:
        case OP_printc:{
                           uint32_t val = va_arg(args, uint32_t);
                           WRITE_LONG(*offset, val);
                           break;
                       }
        case OP_jeq:
        case OP_jne:
        case OP_jgt:
        case OP_jlt:{
                        memory[*offset + 1] = va_arg(args, int);
                        memory[*offset + 2] = va_arg(args, int);
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset + 2, val);
                        break;
                    }
        case OP_jov:
        case OP_jun:{
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        break;
                    }
        case OP_halt:
        case OP_clrpc:
        case OP_clrsr:
                    break;
        case OP_const:{
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        break;
                      }
    }
    *offset += instructionLength[opcode];
    va_end(args);
}

int main(){
    VirtualMachine *machine = rm_new();
    char str[] = "After loop, value of r0 is : ";
    rm_init(machine, 1024);
    uint8_t *ins = (uint8_t *)malloc(sizeof(uint8_t) * 1000);
    uint32_t offset = 0;
    
    // Initialtization
    // mov #0, r0
    write_op(ins, &offset, OP_mov, 0, 0);
    // mov #1, r1
    write_op(ins, &offset, OP_mov, 1, 1);
    // mov #100000000, r2
    write_op(ins, &offset, OP_mov, 100000000, 2);
    // Move string to memory
    for(uint32_t i = 0;str[i] != '\0';i++){
        // save #str[i], @450 + (i*4)
        write_op(ins, &offset, OP_save, str[i], 450 + (i*4));
    }
    // Save the jump offset
    uint32_t jmp = offset;
    // Increment r0
    // add r1, r0
    write_op(ins, &offset, OP_add, 1, 0);
    // Jump while r0 < r2
    // jlt r0, r2, @jmp
    write_op(ins, &offset, OP_jlt, 0, 2, jmp);
    // Print the string
    for(uint32_t i = 0;str[i] != '\0';i++){
        // printc @450 + (i*4)
        write_op(ins, &offset, OP_printc, 450 + (i*4));
    }
    // Save r0 to @446
    // store r0, @446
    write_op(ins, &offset, OP_store, 0, 446);
    // Print the value stored at 111
    // print @446
    write_op(ins, &offset, OP_print, 446);
    // Halt
    // halt
    write_op(ins, &offset, OP_halt);
    
    rm_write_mem(machine, ins, offset, 0);
    offset = 0;
    while(machine->memory[offset] != OP_halt)
        debugInstruction(machine->memory, &offset);
    
    rm_run(machine, 0);

    free(ins);
    rm_free(machine);
    return 0;
}

