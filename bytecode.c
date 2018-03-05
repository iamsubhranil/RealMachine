#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "display.h"

#include "vm.h"
#include "bytecode.h"

void bc_write_byte(uint8_t *memory, uint32_t *offset, uint8_t data){
    memory[*offset] = data;
    (*offset)++;
}

void bc_copy_arr(uint8_t *memory, uint8_t *data, uint32_t size, uint32_t offset){
    for(uint32_t i = 0;i < size;i++)
        memory[i + offset] = data[i];
}

static uint8_t instructionLength[] = {
    #define OPCODE(a, length, b) length,
    #include "opcodes.h"
    #undef OPCODE
};

Data bc_read_from_disk(const char *inputFile){
    return (Data){NULL, 0};
}

bool bc_save_to_disk(const char *outputFile, uint8_t *memory, uint32_t size){ 
    FILE *save = fopen(outputFile, "w");
    if(!save){
        err("Unable to open file for saving : " ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET " !\n", outputFile);
        return false;
    }
#ifdef DEBUG
    dbg("Writing to file '%s'\n", outputFile);
#endif
    fwrite(memory, size, 1, save);
    fclose(save);
    return true;
}

void bc_write_op(uint8_t *memory, uint32_t *offset, int opcode, ...){
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
    Code opc = (Code)opcode;
    switch(opc){
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
        case OP_nex:
                    break;
        case OP_const:{
                          uint32_t val = va_arg(args, uint32_t);
                          WRITE_LONG(*offset, val);
                          break;
                      }
    }
    *offset += instructionLength[opcode];
    va_end(args);
#undef WRITE_LONG
}
