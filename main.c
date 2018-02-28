#include "vm.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void write_op(uint8_t *memory, uint32_t *offset, int opcode, ...){
    memory[*offset] = opcode;

#define WRITE_LONG(o, val) \
    memory[o + 1] = val >> 24; \
    memory[o + 2] = val >> 16; \
    memory[o + 3] = val >> 8; \
    memory[o + 4] = val;

    va_list args;
    va_start(args, opcode);
    switch(opcode){
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_AND:
        case OP_OR:
            memory[*offset + 1] = va_arg(args, int);
            memory[*offset + 2] = va_arg(args, int);
            *offset += 3;
            break;
        case OP_NOT:
            memory[*offset + 1] = va_arg(args, int);
            *offset += 2;
            break;
        case OP_LSHIFT:
        case OP_RSHIFT:{
                           memory[*offset + 1] = va_arg(args, int);
                           uint32_t val = va_arg(args, uint32_t);
                           WRITE_LONG(*offset + 1, val);
                           *offset += 6;
                           break;
                       }
        case OP_LOAD:{
                         uint32_t val = va_arg(args, uint32_t);
                         WRITE_LONG(*offset, val);
                         memory[*offset + 5] = va_arg(args, int);
                         *offset += 6;
                         break;
                     }
        case OP_STORE:{
                          memory[*offset + 1] = va_arg(args, int);
                          uint32_t val = va_arg(args, uint32_t);
                          WRITE_LONG(*offset + 1, val);
                          *offset += 6;
                          break;
                      }
        case OP_MOV:{
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        memory[*offset + 5] = va_arg(args, int);
                        *offset += 6;
                        break;
                    }
        case OP_SAVE:{
                         uint32_t val1 = va_arg(args, uint32_t);
                         uint32_t val2 = va_arg(args, uint32_t);
                         WRITE_LONG(*offset, val1);
                         WRITE_LONG(*offset + 4, val2);
                         *offset += 9;
                         break;
                     }
        case OP_PRINT:
        case OP_PRINTC:{
                           uint32_t val = va_arg(args, uint32_t);
                           WRITE_LONG(*offset, val);
                           *offset += 5;
                           break;
                       }
        case OP_JEQ:
        case OP_JNE:
        case OP_JGT:
        case OP_JLT:{
                        memory[*offset + 1] = va_arg(args, int);
                        memory[*offset + 2] = va_arg(args, int);
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset + 2, val);
                        *offset += 7;
                        break;
                    }
        case OP_JOV:
        case OP_JUN:{
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        *offset += 5;
                        break;
                    }
        case OP_HALT:
        case OP_CLRPC:
        case OP_CLRSR:
                    (*offset)++;
                    break;
    }
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
    write_op(ins, &offset, OP_MOV, 0, 0);
    // mov #1, r1
    write_op(ins, &offset, OP_MOV, 1, 1);
    // mov #100000000, r2
    write_op(ins, &offset, OP_MOV, 100000000, 2);
    // Move string to memory
    for(uint32_t i = 0;str[i] != '\0';i++){
        // save #str[i], @450 + (i*4)
        write_op(ins, &offset, OP_SAVE, str[i], 450 + (i*4));
    }
    // Save the jump offset
    uint32_t jmp = offset;
    // Increment r0
    // add r1, r0
    write_op(ins, &offset, OP_ADD, 1, 0);
    // Jump while r0 < r2
    // jlt r0, r2, @jmp
    write_op(ins, &offset, OP_JLT, 0, 2, jmp);
    // Print the string
    for(uint32_t i = 0;str[i] != '\0';i++){
        // printc @450 + (i*4)
        write_op(ins, &offset, OP_PRINTC, 450 + (i*4));
    }
    // Save r0 to @446
    // store r0, @446
    write_op(ins, &offset, OP_STORE, 0, 446);
    // Print the value stored at 111
    // print @446
    write_op(ins, &offset, OP_PRINT, 446);
    // Halt
    // halt
    write_op(ins, &offset, OP_HALT);
    
    rm_write_mem(machine, ins, offset, 0);
    offset = 0;
    while(machine->memory[offset] != OP_HALT)
        debugInstruction(machine->memory, &offset);
    
    rm_run(machine, 0);

    free(ins);
    rm_free(machine);
    return 0;
}

