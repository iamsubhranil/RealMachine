#include "debug.h"
#include "vm.h"
#include <inttypes.h>
#include <stdio.h>

static const char* opStrings [] = {
    #define OPCODE(name) #name,
    #include "opcodes.h"
    #undef OPCODE
};

static uint8_t *memory = NULL;

//#define READ_BYTE(x) memory[x]

/*uint32_t READ_LONG(uint32_t offset){
    Register r;
    r.byte[3] = memory[offset];
    r.byte[2] = memory[offset + 1];
    r.byte[1] = memory[offset + 2];
    r.byte[0] = memory[offset + 3];
    return r.lng;
}
*/

void debugRegister(VirtualMachine *machine, uint8_t index){
    Register r;
    r.lng = machine->registers[index];
    printf("r%" PRIu8 " : %08x  %08x  %08x  %08x\n", index, r.byte[3], r.byte[2], r.byte[1], r.byte[0]);
}

void debugInstruction(uint8_t *mem, uint32_t *offset){
    printf("%04" PRIu32 "\t", *offset);
    memory = mem;
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
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_AND:
        case OP_OR:
            printf("r%" PRIu8 ",\tr%" PRIu8, READ_BYTE(*offset + 1), READ_BYTE(*offset + 2));
            *offset += 3;
            break;
        case OP_NOT:
            printf("r%" PRIu8, READ_BYTE(*offset + 1));
            *offset += 2;
            break;
        case OP_LSHIFT:
        case OP_RSHIFT:
            printf("r%" PRIu8 ",\t#%" PRIu32, READ_BYTE(*offset + 1), READ_LONG(*offset + 2));
            *offset += 6;
            break;
        case OP_LOAD:
            printf("@%" PRIu32 ",\tr%" PRIu8, READ_LONG(*offset + 1), READ_BYTE(*offset + 5));
            *offset += 6;
            break;
        case OP_STORE:
            printf("r%" PRIu8 ",\t@%" PRIu32, READ_BYTE(*offset + 1), READ_LONG(*offset + 2));
            *offset += 6;
            break;
        case OP_MOV:
            printf("#%" PRIu32 ",\tr%" PRIu8, READ_LONG(*offset + 1), READ_BYTE(*offset + 5));
            *offset += 6;
            break;
        case OP_SAVE:
            printf("#%" PRIu32 ",\t@%" PRIu32, READ_LONG(*offset + 1), READ_LONG(*offset + 5));
            *offset += 9;
            break;
        case OP_PRINT:
        case OP_PRINTC:
            printf("@%" PRIu32, READ_LONG(*offset + 1));
            *offset += 5;
            break;
        case OP_JEQ:
        case OP_JNE:
        case OP_JGT:
        case OP_JLT:
            printf("r%" PRIu8 ",\tr%" PRIu8 ",\t@%" PRIu32, READ_BYTE(*offset + 1), READ_BYTE(*offset + 2), READ_LONG(*offset + 3));
            *offset += 7;
            break;
        case OP_JOV:
        case OP_JUN:
            printf("@%" PRIu8, READ_LONG(*offset + 1));
            *offset += 5;
            break;
        case OP_HALT:
        case OP_CLRPC:
        case OP_CLRSR:
            (*offset)++;
            break;
    }
    printf("\n");

    #undef READ_BYTE
    #undef READ_WORD
    #undef READ_LONG
}
