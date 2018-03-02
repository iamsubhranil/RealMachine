#include "debug.h"
#include "vm.h"
#include <inttypes.h>
#include <stdio.h>

static const char* opStrings [] = {
    #define OPCODE(name, a, b) #name,
    #include "opcodes.h"
    #undef OPCODE
};

static uint32_t instructionLength[] = {
    #define OPCODE(a, length, b) length,
    #include "opcodes.h"
    #undef OPCODE
};
void debugRegister(VirtualMachine *machine, uint8_t index){
    Register r;
    r.lng = machine->registers[index];
    printf("r%" PRIu8 " : %08x  %08x  %08x  %08x\n", index, r.byte[3], r.byte[2], r.byte[1], r.byte[0]);
}

void debugInstruction(uint8_t *memory, uint32_t *offset){
    printf("%04" PRIu32 "\t", *offset);
    uint8_t opcode = memory[*offset];
    if(opcode > sizeof(opStrings)){
        printf("UNKNWN\n");
        (*offset)++;
        return;
    }
    else
        printf("%6s\t", opStrings[opcode]);

    #define READ_BYTE(x) memory[x]
    #define READ_WORD(x) ((READ_BYTE(x) << 8) | (READ_BYTE(x + 1)))
    #define READ_LONG(x) (((READ_WORD(x)) << 16) | (READ_WORD(x + 2)))

    switch(opcode){
        case OP_add:
        case OP_sub:
        case OP_mul:
        case OP_div:
        case OP_and:
        case OP_or:
            printf("r%" PRIu8 ",\tr%" PRIu8, READ_BYTE(*offset + 1), READ_BYTE(*offset + 2));
            break;
        case OP_not:
            printf("r%" PRIu8, READ_BYTE(*offset + 1));
            break;
        case OP_lshift:
        case OP_rshift:
            printf("r%" PRIu8 ",\t#%" PRIu32, READ_BYTE(*offset + 1), READ_LONG(*offset + 2));
            break;
        case OP_load:
            printf("@%" PRIu32 ",\tr%" PRIu8, READ_LONG(*offset + 1), READ_BYTE(*offset + 5));
            break;
        case OP_store:
            printf("r%" PRIu8 ",\t@%" PRIu32, READ_BYTE(*offset + 1), READ_LONG(*offset + 2));
            break;
        case OP_mov:
            printf("#%" PRIu32 ",\tr%" PRIu8, READ_LONG(*offset + 1), READ_BYTE(*offset + 5));
            break;
        case OP_save:
            printf("#%" PRIu32 ",\t@%" PRIu32, READ_LONG(*offset + 1), READ_LONG(*offset + 5));
            break;
        case OP_print:
        case OP_printc:
            printf("@%" PRIu32, READ_LONG(*offset + 1));
            break;
        case OP_jeq:
        case OP_jne:
        case OP_jgt:
        case OP_jlt:
            printf("r%" PRIu8 ",\tr%" PRIu8 ",\t@%" PRIu32, READ_BYTE(*offset + 1), READ_BYTE(*offset + 2), READ_LONG(*offset + 3));
            break;
        case OP_jov:
        case OP_jun:
            printf("@%" PRIu8, READ_LONG(*offset + 1));
            break;
        case OP_halt:
        case OP_clrpc:
        case OP_clrsr:
            break;
        case OP_const:
            printf("#%" PRIu32, READ_LONG(*offset + 1));
            break;
        case OP_nex:
            printf("Error : Code not executable!");
            break;
    }
    *offset += instructionLength[opcode];
    printf("\n");

    #undef READ_BYTE
    #undef READ_WORD
    #undef READ_LONG
}
