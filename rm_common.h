#pragma once

// Turns on debug message from various modules
// while execution.
//
// #define DEBUG

// Turns on instruction stepping while execution
//
// #define DEBUG_INSTRUCTIONS

// This macro sanitizes all memory access, i.e.
// read and writes, by checking whether they are
// out of bounds at each access explicitly.
// Needless to say, this slows down the machine
// to a large degree. Hence, it is defined here
// as a switch between performance and security.
//
// #define SANITIZE_ACCESS

// This macro switches between computed goto
// and traditional switch case for instruction
// dispatch. Generally, computed gotos are a lot 
// faster.
#define REAL_COMPUTED_GOTO

// To allow scan time messages in '()'
#define RM_ALLOW_LEXER_MESSAGES
// To allow parse time messages in '{}'
#define RM_ALLOW_PARSE_MESSAGES
