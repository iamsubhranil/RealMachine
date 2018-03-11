

#if defined(DEBUG) || defined(DEBUG_INSTRUCTIONS)

#include "debug.h"
#include "vm.h"
#include "display.h"

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
#define preg(x) pcyn("r%" PRIu8, READ_BYTE(x))
#define pmem(x) pylw("@%" PRIu32, READ_LONG(x))
#define pimm(x) pmgn("#%" PRIu32, READ_LONG(x))
#define pcmm() printf(",\t")

void debugInstruction(uint8_t *memory, uint32_t *offset, uint32_t size){
    if(size <= (*offset) || size <= (*offset) + instructionLength[memory[*offset]]){
        *offset = size;
        return;
    }
    printf(ANSI_FONT_BOLD);
    pblue("%04" PRIu32 "\t", *offset);
    uint8_t opcode = memory[*offset];
    
    printf(ANSI_FONT_BOLD);
    if(opcode > sizeof(opStrings)){
        pred("UNKNWN\n");
        (*offset)++;
        return;
    }
    else
        pgrn("%6s\t", opStrings[opcode]);

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
        case OP_rcopy:
            // reg --> cyan
            // mem --> yellow
            // imm --> magenta
            preg(*offset + 1);
            pcmm();
            preg(*offset + 2);
            break;
        case OP_not:
            preg(*offset + 1);
            break;
        case OP_lshift:
        case OP_rshift:
            preg(*offset + 1);
            pcmm();
            pimm(*offset + 2);
            break;
        case OP_load:
            pmem(*offset + 1);
            pcmm();
            preg(*offset + 5);
            break;
        case OP_store:
            preg(*offset + 1);
            pcmm();
            pmem(*offset + 2);
            break;
        case OP_mov:
            pimm(*offset + 1);
            pcmm();
            preg(*offset + 5);
            break;
        case OP_save:
            pimm(*offset + 1);
            pcmm();
            pmem(*offset + 5);
            break;
        case OP_print:
        case OP_printc:
            pmem(*offset + 1);
            break;
        case OP_jeq:
        case OP_jne:
        case OP_jgt:
        case OP_jlt:
            preg(*offset + 1);
            pcmm();
            preg(*offset + 2);
            pcmm();
            pmem(*offset + 3);
            break;
        case OP_jov:
        case OP_jun:
            pmem(*offset + 1);
            break;
        case OP_halt:
        case OP_clrpc:
        case OP_clrsr:
            break;
        case OP_const:
            pimm(*offset + 1);
            break;
        case OP_nex:
            pred("[Error] Code not executable!");
            break;
        case OP_mcopy:
            pmem(*offset + 1);
            pcmm();
            pmem(*offset + 5);
            break;
        case OP_jmp:
            pmem(*offset + 1);
            break;
    }
    *offset += instructionLength[opcode];
    printf("\n");

    #undef READ_BYTE
    #undef READ_WORD
    #undef READ_LONG
}

#endif
