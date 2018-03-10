#pragma once
#include "rm_common.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum{
    #define ET(x) TOKEN_##x
    ET(comma),
    ET(register),
    ET(hash),
    ET(address),
    ET(number),
    ET(eof),
    ET(unknown),
    ET(label),
    ET(colon),
    ET(aphostrophy),
#ifdef RM_ALLOW_PARSE_MESSAGES
    ET(parseMessage),
#endif
    #define OPCODE(x, a, b) ET(x),
    #include "opcodes.h"
    #undef OPCODE
    #undef ET
} TokenType;

typedef struct{
    char* string;
    char* source;
    uint8_t length;
    size_t line;
    size_t start;
    size_t end;
    TokenType type;
} Token;

typedef struct{
    char *source;
    Token *tokens;
    uint32_t count;
    uint32_t hasError;
} TokenList;

TokenList tokens_scan(const char *source);
void tokens_free(TokenList list);
void token_print_source(Token t, uint8_t reportType);
//#define DEBUG

#ifdef DEBUG
void lexer_print_tokens(TokenList list);
#endif
