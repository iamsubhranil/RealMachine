it will be a register based machine. source code will be assembly like. which will be converted into bytecode and stored on a virtual memory starting from location 0. the source code can be like this

mov #32, r0 //move immediate long 32 to r0
mov #14, r1 //move immediate long 14 to r1
jmp @14 //jump to offset 14. since in the next line we're storing a data instead of an instruction, we want to skip executing it accidentally.
savei #0 //save long 0. while converting to bytecode, at this particular offset in memory, instead of an instruction, a long value of 0 will be stored. let's call this offset 10.
add r0, r1 // add r0 r1 and store it back to r1. let's call this location 14.
store r0, @10 // store a long word from r0 to memory offset 10, where we previously stored some data instead of an instruction.
print @10 //print long 10. print a long word from memory location 10.

Register Set
=========
There will be 8 32bit GPRs, r0 to r7, which can be accessed by either the upper half, upper 8 bits, or their lower counterparts, or by the whole. There will be two special purpose registers, namely the program counter and status register. this registers cannot be directly accessed. however, they can be accessed by means of some instructions.
PC will keep track of the memory location to execute next, and hence will be of 32 bits.
SR will keep track of the result of various exceptional situations. if results of arithmetic operations become anamolus, i.e. by an overflow or underflow, that will be indicated here. the size of this register will be 8 bits.

To clear the special registers, one can use the following
clrpc // clears the pc, i.e. sets it to 0
clrsr // clears the sr, i.e. sets it to 0

/* NOT USED
   ========

    Arithmetic Comparison
    ==================
    There will be three comparison operation, ce, cne, cgt, and clt, which will perform comparisons between two registers, and set the SR as 
    following : 
    1 if dest1 == dest2
    2 if dest1 != dest2
    3 if dest1 > dest2
    4 if dest1 < dest2
    if the result of any comparison is false, then the SR will reset.
*/

Branching
========
all branching instructions will take two register operands as source, and will take another operand, a memory address, which will tell the vm where to branch incase the implied condition is true.
jeq : jump if the comparison produced "equal"
jne : jump if the comparison produced "unequal"
jgt : jump if the comparison produced "greater than"
jlt : jump if the comparison produced "lesser than"

These two special jump instructions will work on the status register, if the program wants to take action to some anamolus situations
jov : jump if SR == 1, i.e. the last arithmetic op was "overflow"ed
jun : jump if SR == 2, i.e. the last arithmetic op was "underflow"ed

there will also be a generic jmp instruction which will set the PC to a particular value without performing any conditional check.
syntax will be like

jne r1, r2, @16 // jump to offset 16 if the r1 != r2
jgt r6, r7, @32 // jump to offset 16 if r6 > r7
jmp @20 // jump to offset 20 unconditionally

Arithmetic Operations
=================
All the usual set of arithmetic operations, i.e. addition multiplication division and subtraction will be present. however, they will only work on registers. the can however, work on a byte, an word, or a long as specified by the instruction. the instructions will usually work on long words, their word counterparts will have a 'w' written in the end, and their byte counterparts will have 'b' at the end. the result will be stored in the second operand implicitly, and the specified register will be overwritten by that result 

add r0, r1 // add r0 and r1 and store it to r1
subw r2, r3 // subtract a word from r2 ro r3 and store it to r3

these operations will implicitly either reset the OR, or set it as required.

There will also be a set of bitwise operations that will work on two registers in the exact same way, and they are AND, OR, XOR. the result of these operations will also be stored on the second operand. there will also be a NOT operation, which will work on a single register and perform bitwise inversion, storing the result in the same register. there will also be usual word and long versions of the same.

and r0, r1
orw r1, r2
not r4

There will be a set of bitshift operations, which will move the bits of the specific register to a specific position left or right. any new bit introduced in the register as a result of these shifts will always be 0. syntax will be like the following

rshift r3, #6 // shift the bits of register 3 by 6 places right
lshift r2, #10 // shift the bits of register 2 by 10 places left

Theoretically the second operand, i.e. the shift_by operand can be as long as a byte, but anything greater than 32 will always result the specific register being set to 0.

Data Movement
============
All the necessary data will be stored in memory, however, all the operations will be performed on the registers. There will be two instructions for both sided movement, and their usual 'w' and 'b' counterparts will be present. Their syntax will be 
ins source dest
To move a piece of data from memory to a register, one should use "load", as in
load @37, r0 // load a long value from offset 37 to r0
loadb @23, r4 // load a byte from offset 23 to r4
loadw @12, r6 // load a word from offset 12 to 6
To move a piece of data from register to memory, one should use "store", as in
store r0, @23 // store a long value from r0 to offset 23
and so on. Storing anything will result an overwrite at that particular location, and storing words or longs will result overwrite in the subsequent locations, so use with caution.
To load a value to a register directly, one should use "mov", as in
mov #3773, r0 // move the value 3773 to register 0
This will also have it's b and w counterparts.
To store a value in memory directly, one can use "save", as in
save #2362, @46 // save the value 2362 at offset 46
To store a value in the bytecode directly at the present location, one can use "savei", as in
savei #2674 // store 2674 in present offset
As usual, these too, will have their b and w counterparts.

I/O
==
Right now, there is no I, only O. The most guessable instruction of all, "print" will work only on the memory locations. It will have four variants, one each for the usual l b and w, and another as "printc". It will read a byte from the specified location, and print it as a character.
print @23 // print a long value from offset 23
printc @28 // print a byte from offset 28 as character

Instruction length
==============
All opcodes will be of size 8 bits, or one byte. A word will occupy two bytes, and a long will occupy 4 bytes of space. A register index operand will occupy 8 bits, and an offset operand will occupy 4 bytes of space. The bitshift operation will take a byte as the shift length.

Syntax
======
To make the language more understandable,
register operands will precede 'r'
immediate constant operands will precede '#'
memory offsets will precede '@'
and there will be a comma between two operands, like :

load #37, r0
save #2362, @12
lshift r0, #2

The parser and code generator will be implemented later, after the core vm is designed, hence the syntax can vary over time for now.
