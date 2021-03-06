/* The OPCODE macro should be of format
 * 
 * OPCODE(name, instruction_length, opcode_string_length)
 * 
 * The instruction length should include the length of
 * the opcode itself, i.e. 1 byte
 */


// add r0, r1
OPCODE(add, 3, 3)

// sub r0, r1
OPCODE(sub, 3, 3)

// mul r0, r1
OPCODE(mul, 3, 3)

// div r0, r1
OPCODE(div, 3, 3)

// and r0, r1
OPCODE(and, 3, 3)

// or r0, r1
OPCODE(or, 3, 2)

// not r0
OPCODE(not, 2, 3)

// lshift r0, #21
OPCODE(lshift, 6, 6)

// rshift r0, #21
OPCODE(rshift, 6, 6)

// load @37, r0
OPCODE(load, 6, 4)

// store r0, @43
OPCODE(store, 6, 5)

// mov #4343, r0
OPCODE(mov, 6, 3)

// save #4293, @32
OPCODE(save, 9, 4)

// print @443
OPCODE(print, 5, 5)

// printc @32
OPCODE(printc, 5, 6)

// jeq r1, r2, @32
OPCODE(jeq, 7, 3)

// jne r1, r2, @32
OPCODE(jne, 7, 3)

// jgt r1, r2, @32
OPCODE(jgt, 7, 3)

// jlt r1, r2, @32
OPCODE(jlt, 7, 3)

// jov @32
OPCODE(jov, 4, 3)

// jun @32
OPCODE(jun, 4, 3)

// clrpc
OPCODE(clrpc, 1, 5)

// clrsr
OPCODE(clrsr, 1, 5)

// halt
OPCODE(halt, 1, 4)

// const #323
OPCODE(const, 4, 5)

// mark an offset as non-executable memory
// this is not accessible by a source program,
// only to use in internals
OPCODE(nex, 1, 0)

// stores a string constant at present
// offset.
// supported escape sequences :
// \n : newline
// \t : tab
// \" : "
// All characters will consume a byte
// each, and escape sequences will consume
// one byte as a whole
// For example,
// "Hello\tWorld!" will consume
// 12 bytes
//
// str "My\nName\nIs\n : \t"
OPCODE(str, 1, 3)

// Copies the value stored at a memory offset
// to another
//
// mcopy @offset_from, @offset_to
OPCODE(mcopy, 9, 5)

// Copies the value stored at one register
// to another
//
// rcopy r_from, r_to
OPCODE(rcopy, 3, 5)

// F**K me
//
// jmp @offset
OPCODE(jmp, 5, 3)

// Increments the value stored
// at a register by 1
//
// incr r0
OPCODE(incr, 2, 4)

// Decrements the value stored
// at a register by 1
//
// decr r0
OPCODE(decr, 2, 4)

// Prints a number of bytes as characters
// starting from a given memory offset
// The following prints 23 consecutive
// bytes starting from given offset
//
// prints @offset, #23
OPCODE(prints, 9, 6)
