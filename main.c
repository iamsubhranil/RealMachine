#include "vm.h"
#include "debug.h"
#include "bytecode.h"
#include "lexer.h"
#include "parser.h"
#include "display.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

int main(){
    VirtualMachine *machine = rm_new();
    rm_init(machine, 1024);
    char source[] = "mov #0, r0"
        "\nmov #00000000, r1"
        "\nmov #1, r2"
        "\nloop : add r2, r0"
        "\njlt r0, r1, @loop"
        "\nhalt"
        "\nvar : const #0";
#ifdef DEBUG
    dbg("===== Source =====");
    printf("\n%s\n", source);
    dbg("===== Scanning =====\n");
#endif
    TokenList l = tokens_scan(source);
#ifdef DEBUG
    dbg("===== Tokens =====\n");
    lexer_print_tokens(l);
    printf("\n");
    dbg("===== Parsing and Compiling =====\n");
#endif
    if(l.hasError==0){
        if(parse_and_emit(l, source, machine->memory, 1024, 0)){
#ifdef DEBUG
            uint32_t offset = 0;
            printf("\n");
            dbg("===== Compiled chunk =====\n");
            while(machine->memory[offset] != OP_nex)
                debugInstruction(machine->memory, &offset);
            printf("\n");
            dbg("===== Executing =====\n");
#endif
            fflush(stdout);
            rm_run(machine, 0);
            printf("\n");
        }
        else
            err("Unable to start virtual machine!\n");
    }
    else{
        err("Scanning completed with " ANSI_FONT_BOLD ANSI_COLOR_RED "%" PRIu32  ANSI_COLOR_RESET " errors!", l.hasError); 
        err("Unable to start virtual machine!\n");
    }
#ifdef DEBUG
    dbg("===== Execution Complete =====\n");
#endif
    rm_free(machine);
    tokens_free(l);
}

int test(){
    VirtualMachine *machine = rm_new();
    char str[] = "After loop, value of r0 is : ";
    rm_init(machine, 1024);
    uint32_t offset = 0;

    // Initialtization
    // mov #0, r0
    bc_write_op(machine->memory, &offset, OP_mov, 0, 0);
    // mov #1, r1
    bc_write_op(machine->memory, &offset, OP_mov, 1, 1);
    // mov #100000000, r2
    bc_write_op(machine->memory, &offset, OP_mov, 100000000, 2);
    // Move string to memory
    for(uint32_t i = 0;str[i] != '\0';i++){
        // save #str[i], @450 + (i*4)
        bc_write_op(machine->memory, &offset, OP_save, str[i], 450 + (i*4));
    }
    // Save the jump offset
    uint32_t jmp = offset;
    // Increment r0
    // add r1, r0
    bc_write_op(machine->memory, &offset, OP_add, 1, 0);
    // Jump while r0 < r2
    // jlt r0, r2, @jmp
    bc_write_op(machine->memory, &offset, OP_jlt, 0, 2, jmp);
    // Print the string
    for(uint32_t i = 0;str[i] != '\0';i++){
        // printc @450 + (i*4)
        bc_write_op(machine->memory, &offset, OP_printc, 450 + (i*4));
    }
    // Save r0 to @446
    // store r0, @446
    bc_write_op(machine->memory, &offset, OP_store, 0, 446);
    // Print the value stored at 111
    // print @446
    bc_write_op(machine->memory, &offset, OP_print, 446);
    // Halt
    // halt
    bc_write_op(machine->memory, &offset, OP_halt);

    offset = 0;
    while(machine->memory[offset] != OP_halt)
        debugInstruction(machine->memory, &offset);

    rm_run(machine, 0);

    free(machine->memory);
    rm_free(machine);
    return 0;
}

