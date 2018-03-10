#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
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

/* Bytecode format
 * ---------------
 * MAGIC --> 32 bits
 * Bytecode VERSION -->  8 bits
 * core bytecode length excluding header and footer --> 32 bits
 * HEADER --> 32 bits
 * core bytecode
 * FOOTER --> 32 bits
 */

typedef struct{
    uint8_t version;
    uint8_t *code;
    uint32_t magic;
    uint32_t length;
    uint32_t header;
    uint32_t footer;
} Bytecode;

#define MAGIC 0x726c6d63 // rlmc
#define HEADER 0x62737274 // bsrt
#define FOOTER 0x62656e64 // bend

// This will change if the bytecode format is updated
#define CURRENT_EXECUTABLE_VERSION 0x1

#ifdef DEBUG
#define SHOW_FAIL(x, str) {\
        printf(" (" ANSI_COLOR_RED ANSI_FONT_BOLD "FAILED" ANSI_COLOR_RESET ") "); \
        dbg("Expected " str " : " ANSI_COLOR_RED ANSI_FONT_BOLD "0x%x" ANSI_COLOR_RESET, x); \
    }
#define STARTMSG(str, val) {\
    dbg(str " : 0x%x", val);}
#define DONEMSG() {\
    printf(" (" ANSI_COLOR_GREEN ANSI_FONT_BOLD "PASSED" ANSI_COLOR_RESET ")"); }
#else
#define SHOW_FAIL(a, b) {}
#define STARTMSG(str, val) {}
#define DONEMSG() {}
#endif

#define VERIFY(x, y, str, msg) \
    STARTMSG(str, y); \
    if(x != y) {\
        SHOW_FAIL(x, str); \
        err(msg); \
        goto stopread; \
    } \
    DONEMSG();


Data bc_read_from_disk(const char *inputFile){
    FILE *opn = fopen(inputFile, "rb");
    if(!opn){
        err("Unable to open file for reading : " ANSI_COLOR_RED ANSI_FONT_BOLD 
                "%s" ANSI_COLOR_RESET "!\n", inputFile);
        return (Data){NULL, 0};
    }
    fseek(opn, 0L, SEEK_END);
    long size = ftell(opn);
    fseek(opn, 0L, SEEK_SET);
#ifdef DEBUG
    dbg("File size : " ANSI_COLOR_CYAN ANSI_FONT_BOLD "%ld" ANSI_COLOR_RESET " bytes\n", size);
#endif
    if(size < 18){ // 21 for metadata, atleast 1 opcode
        err("Size of the executable is less than expected!");
            goto stopread;
    }
    Bytecode bc;
    bc.code = NULL;

    fread(&bc.magic, 4, 1, opn);
    fread(&bc.version, 1, 1, opn);
    fread(&bc.length, 4, 1, opn);
    fread(&bc.header, 4, 1, opn);

#ifdef DEBUG
    dbg("===== Verifying File Metadata =====");
#endif

    VERIFY(MAGIC, bc.magic, "Magic", "Not a valid RealMachine executable!");
    VERIFY(CURRENT_EXECUTABLE_VERSION, bc.version, "Version", "This version of the executable is "
            "not supported by the program!");
    VERIFY(bc.length + 17, size, "Total size", "The executable is corrupted!");
    VERIFY(HEADER, bc.header, "Header", "The executable is corrupted!");
    
    fseek(opn, bc.length, SEEK_CUR);
    fread(&bc.footer, 4, 1, opn);
    
    VERIFY(FOOTER, bc.footer, "Footer", "The executable is corrupted!");

#ifdef DEBUG
    dbg("Reading bytecode");
#endif

    // Everything is good
    fseek(opn, 13, SEEK_SET);
    bc.code = (uint8_t *)malloc(sizeof(uint8_t) * (bc.length+1));
    fread(bc.code, bc.length, 1, opn);

#ifdef DEBUG
    dbg("Read complete!\n");
#endif

stopread:
    fclose(opn);
    return (Data){bc.code, bc.code == NULL ? 0 : bc.length};
}

bool bc_save_to_disk(const char *outputFile, uint8_t *memory, uint32_t size){ 
    FILE *save = fopen(outputFile, "w");
    if(!save){
        err("Unable to open file for saving : " ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET " !\n", outputFile);
        return false;
    }

    Bytecode bc;
    bc.code = memory;
    bc.length = size;
    bc.magic = MAGIC;
    bc.header = HEADER;
    bc.footer = FOOTER;
    bc.version = CURRENT_EXECUTABLE_VERSION;

#ifdef DEBUG
    dbg("File size : " ANSI_FONT_BOLD ANSI_COLOR_CYAN "%ld" ANSI_COLOR_RESET " bytes\n", size+17);
    dbg("===== Writing File Metadata =====");
    dbg("Magic : 0x%x", bc.magic);
    dbg("Version : 0x%x", bc.version);
    dbg("Code length : %" PRIu32 " bytes", bc.length);
    dbg("Header : 0x%x", bc.header);
    dbg("Footer : 0x%x", bc.footer);
#endif

    fwrite(&bc.magic, 4, 1, save);
    fwrite(&bc.version, 1, 1, save);
    fwrite(&bc.length, 4, 1, save);
    fwrite(&bc.header, 4, 1, save);
    fwrite(bc.code, size, 1, save);
    fwrite(&bc.footer, 4, 1, save);
    fclose(save);
    return true;
}

void bc_write_op(uint8_t *memory, uint32_t *offset, int opcode, ...){
    if(opcode == OP_const || opcode == OP_char)
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
        case OP_char:{
                        uint8_t ch = va_arg(args, int);
                        memory[*offset + 1] = ch;
                        break;
                     }
        case OP_mcopy:
                     {
                        uint32_t val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset, val);
                        val = va_arg(args, uint32_t);
                        WRITE_LONG(*offset + 4, val);
                        break;
                     }
    }
    *offset += instructionLength[opcode];
    va_end(args);
#undef WRITE_LONG
}
