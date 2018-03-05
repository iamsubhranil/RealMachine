#include "vm.h"
#include "debug.h"
#include "bytecode.h"
#include "lexer.h"
#include "parser.h"
#include "display.h"

#ifdef DEBUG
#include <time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#ifdef DEBUG

static void printTime(clock_t start, clock_t end, const char *job){
    dbg( ANSI_FONT_BOLD ANSI_COLOR_CYAN "%s " ANSI_COLOR_RESET 
            "took " ANSI_FONT_BOLD ANSI_COLOR_GREEN "%f" ANSI_COLOR_RESET
            " seconds", job, (double)(end - start)/CLOCKS_PER_SEC);
}

#endif

int main(){
    VirtualMachine *machine = rm_new();
    rm_init(machine, 1024);
    char source[] = "mov #0, r0"
        "\nmov #100000000, r1"
        "\nmov #1, r2"
        "\nloop : add r2, r0"
        "\njlt r0, r1, @loop"
        "\nstore r0, @var"
        "\nprint @var"
        "\nhalt"
        "\nvar : const #0";
#ifdef DEBUG
    dbg("===== Source =====");
    printf("\n%s\n", source);
    dbg("===== Scanning =====\n");
    clock_t start = clock();
#endif
    TokenList l = tokens_scan(source);
#ifdef DEBUG
    clock_t end = clock();
    printTime(start, end, "Scanning");
    dbg("===== Tokens =====\n");
    lexer_print_tokens(l);
    printf("\n");
    dbg("===== Parsing and Compiling =====\n");
    start = clock();
#endif
    if(l.hasError==0){
        if(parse_and_emit(l, machine->memory, 1024, 0)){
#ifdef DEBUG
            end = clock();
            printTime(start, end, "Parsing");
            uint32_t offset = 0;
            printf("\n");
            dbg("===== Compiled chunk =====\n");
            while(machine->memory[offset] != OP_nex)
                debugInstruction(machine->memory, &offset);
            printf("\n");
            dbg("===== Executing =====\n");
#endif
            fflush(stdout);
#ifdef DEBUG
            start = clock();
#endif
            rm_run(machine, 0);
#ifdef DEBUG
            end = clock();
            printTime(start, end, "Execution");
#endif
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
