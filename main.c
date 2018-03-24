#include "rm_common.h"
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef DEBUG

static void printTime(clock_t start, clock_t end, const char *job){
    dbg( ANSI_FONT_BOLD ANSI_COLOR_CYAN "%s " ANSI_COLOR_RESET 
            "took " ANSI_FONT_BOLD ANSI_COLOR_GREEN "%f" ANSI_COLOR_RESET
            " seconds", job, (double)(end - start)/CLOCKS_PER_SEC);
}

#endif

static char* read_whole_file(const char* fileName){
    struct stat statbuf;
    stat(fileName, &statbuf);
    if(S_ISDIR(statbuf.st_mode)){
        err("Given argument is a directory!");
        return NULL;
    }
    char *buffer = NULL;
    long length;
    FILE *f = fopen(fileName, "rb");
    if(f){
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
#ifdef DEBUG
        dbg("Buffer allocated of length %ld bytes", length + 1);
#endif
        buffer = (char *)malloc(length + 1);
        if(buffer){
            fread(buffer, 1, length, f);
            buffer[length] = '\0';
        }
        fclose(f);
        return buffer;
    }
    return NULL;
}

/* -r : compiles and runs a source file
 * -e : executes a binary file
 * -c : compiles and saves a source file
 *
 *  Additional arguments must be provided to
 *  denote the input file and/or output file
 *  as required after the options
 */

static void usage(const char *name){
    pgrn(ANSI_FONT_BOLD "\nUsage : " ANSI_COLOR_RESET);
    printf(ANSI_FONT_BOLD "\n1. Run a source file directly\n" ANSI_COLOR_RESET);
    pylw("%s -r input_file", name);
    printf(ANSI_FONT_BOLD "\n2. Compile and save to an executable file\n" ANSI_COLOR_RESET);
    pylw("%s -c input_file output_file", name);
    printf(ANSI_FONT_BOLD "\n3. Run a compiled executable\n" ANSI_COLOR_RESET);
    pylw("%s -e input_file\n", name);
}

int main(int argc, char *argv[]){

    // Argument parsing

    if(argc != 3 && argc != 4){
        err("Wrong arguments!");
        usage(argv[0]);
        return 1;
    }

    int opt, mode = 0;
    char *source = NULL, *outputFile = NULL;
    Data binaryData = (Data){NULL, 0}; // Bytecode container
    
    while((opt = getopt(argc, argv, "rec")) != -1){
        switch(opt){
            case 'r':
                mode += 3;
                break;
            case 'e':
                mode += 5;
                break;
            case 'c':
                mode += 7;
                break;
            default:
end:
                err("Wrong arguments!");
                usage(argv[0]);
                return 1;
        }
    }
    if(mode != 3 && mode != 5 && mode != 7){
        goto end;
    }
    switch(mode){
        case 3:
            if(optind >= argc){
                err("Give a file to execute!");
                usage(argv[0]);
                return 1;
            }
            else if(optind == (argc - 1)){
                source = read_whole_file(argv[optind]);
                if(source == NULL){
                    err("Unable to read input file : " 
                            ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "\n", argv[optind]);
                    return 1;
                }
            }
            else{
                err("Wrong arguments!");
                usage(argv[0]);
                return 1;
            }
            break;
        case 7:
            if(optind >= argc){
                err("Give a file to save the executable!");
                usage(argv[0]);
                return 1;
            }
            else{
                if(optind == (argc - 2)){
                    source = read_whole_file(argv[optind]);
                    if(source == NULL){
                        err("Unable to read input file : " 
                                ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "\n", argv[optind]);
                        return 1;
                    }
                    outputFile = argv[++optind];
                }
                else{
                    err("Must give input and output files!");
                    usage(argv[0]);
                    return 1;
                }
            }
            break;
        case 5:
            if(optind >= argc){
                err("Give a file to execute!");
                usage(argv[0]);
                return 1;
            }
            else if(optind == (argc - 1)){
                binaryData = bc_read_from_disk(argv[optind]);
                if(binaryData.size == 0){
                    err("Unable to start virtual machine!\n");
                    return 1;
                }
            }
            else{
                err("Wrong arguments!");
                usage(argv[0]);
                return 1;
            }
            break;
    }

#ifdef DEBUG
    clock_t start, end; // timers
#endif

    // Main execution starts here
    TokenList l;
    VirtualMachine *machine = rm_new();
    
    if(binaryData.size == 0)
        rm_init(machine, 0);
    else{
        machine->memory = binaryData.memory;
        machine->memSize = binaryData.size;
        goto execute;
    }

#ifdef DEBUG
    dbg("===== Source =====");
    printf("\n%s\n", source);
    dbg("===== Scanning =====\n");
    start = clock();
#endif

    l = tokens_scan(source);

#ifdef DEBUG
    end = clock();
    printTime(start, end, "Scanning");
    dbg("===== Tokens =====\n");
    lexer_print_tokens(l);
    printf("\n");
    dbg("===== Parsing and Compiling =====\n");
    start = clock();
#endif

    if(l.hasError==0){
        if(parse_and_emit(l, &machine->memory, &machine->memSize, 0)){

#ifdef DEBUG
            end = clock();
            printTime(start, end, "Parsing");
            printf("\n");
#endif

execute:;

#ifdef DEBUG
            uint32_t offset = 0;
            dbg("===== %s chunk =====\n", binaryData.size == 0 ? "Compiled" : "Read");
            pblue( ANSI_FONT_BOLD "\nOffset\t");
            pgrn(ANSI_FONT_BOLD "Opcode\t");
            pylw( ANSI_FONT_BOLD "Arguments\n");
            pblue( ANSI_FONT_BOLD "======\t");
            pgrn( ANSI_FONT_BOLD "======\t");
            pylw( ANSI_FONT_BOLD "=========\n");
            while(offset < machine->memSize && machine->memory[offset] != OP_nex)
                debugInstruction(machine->memory, &offset, machine->memSize);
            printf("\n");
            if(!outputFile)
                dbg("===== Executing =====\n");
#endif

            fflush(stdout);
            if(outputFile){

#ifdef DEBUG
                dbg("===== Saving ======\n");
#endif          

                if(bc_save_to_disk(outputFile, machine->memory, machine->memSize))
                    printf(ANSI_COLOR_GREEN ANSI_FONT_BOLD "\n[Done] " ANSI_COLOR_RESET
                            "Compiled and saved to file : " 
                            ANSI_COLOR_CYAN ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "!\n", outputFile);
                else
                    err("Unable to save to given file!\n");
                goto done;
            }

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
        else if(!outputFile)
            err("Unable to start virtual machine!\n");
        else
            err("Unable to save to given file!\n");
    }
    else{
        err("Scanning completed with " ANSI_FONT_BOLD ANSI_COLOR_RED "%" PRIu32  ANSI_COLOR_RESET " errors!", l.hasError); 
        if(!outputFile)
            err("Unable to start virtual machine!\n");
        else
            err("Unable to save to given file!\n");    
    }
done:

#ifdef DEBUG
    dbg("===== Execution Complete =====\n");
#endif

    if(binaryData.size == 0){
        free(source);
        tokens_free(l);
    }
    rm_free(machine);
}
