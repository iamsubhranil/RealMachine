#include "vm.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int main(){
    VirtualMachine *machine = rm_new();
    char str[] = "After loop, value of r0 is : ";
    rm_init(machine, 1024);
    uint32_t offset = 0;
    
    // Initialtization
    // mov #0, r0
    rm_write_op(machine->memory, &offset, OP_mov, 0, 0);
    // mov #1, r1
    rm_write_op(machine->memory, &offset, OP_mov, 1, 1);
    // mov #100000000, r2
    rm_write_op(machine->memory, &offset, OP_mov, 100000000, 2);
    // Move string to memory
    for(uint32_t i = 0;str[i] != '\0';i++){
        // save #str[i], @450 + (i*4)
        rm_write_op(machine->memory, &offset, OP_save, str[i], 450 + (i*4));
    }
    // Save the jump offset
    uint32_t jmp = offset;
    // Increment r0
    // add r1, r0
    rm_write_op(machine->memory, &offset, OP_add, 1, 0);
    // Jump while r0 < r2
    // jlt r0, r2, @jmp
    rm_write_op(machine->memory, &offset, OP_jlt, 0, 2, jmp);
    // Print the string
    for(uint32_t i = 0;str[i] != '\0';i++){
        // printc @450 + (i*4)
        rm_write_op(machine->memory, &offset, OP_printc, 450 + (i*4));
    }
    // Save r0 to @446
    // store r0, @446
    rm_write_op(machine->memory, &offset, OP_store, 0, 446);
    // Print the value stored at 111
    // print @446
    rm_write_op(machine->memory, &offset, OP_print, 446);
    // Halt
    // halt
    rm_write_op(machine->memory, &offset, OP_halt);
    
    rm_write_mem(machine, machine->memory, offset, 0);
    offset = 0;
    while(machine->memory[offset] != OP_halt)
        debugInstruction(machine->memory, &offset);
    
    rm_run(machine, 0);

    free(machine->memory);
    rm_free(machine);
    return 0;
}

