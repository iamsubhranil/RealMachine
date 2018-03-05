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


#ifdef DEBUG

static void printTime(clock_t start, clock_t end, const char *job){
    dbg( ANSI_FONT_BOLD ANSI_COLOR_CYAN "%s " ANSI_COLOR_RESET 
            "took " ANSI_FONT_BOLD ANSI_COLOR_GREEN "%f" ANSI_COLOR_RESET
            " seconds", job, (double)(end - start)/CLOCKS_PER_SEC);
}

#endif

static char* read_whole_file(const char* fileName){
    char *buffer = NULL;
    long length;
    FILE *f = fopen(fileName, "rb");
    if(f){
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
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
                    err("Unable to read input file : " ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "\n", argv[optind]);
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
                        err("Unable to read input file : " ANSI_COLOR_RED ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "\n", argv[optind]);
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
                info("Run from binary is not yet implemented! Check again soon!\n");
            }
            else{
                err("Wrong arguments!");
                usage(argv[0]);
                return 1;
            }
            return 1;
    }

    // Main execution starts here

    VirtualMachine *machine = rm_new();
    rm_init(machine, 0);
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
        if(parse_and_emit(l, &machine->memory, &machine->memSize, 0)){
#ifdef DEBUG
            end = clock();
            printTime(start, end, "Parsing");
            uint32_t offset = 0;
            printf("\n");
            dbg("===== Compiled chunk =====\n");
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
    free(source);
    rm_free(machine);
    tokens_free(l);
}
