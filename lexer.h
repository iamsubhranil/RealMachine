#pragma once

#include <stdint.h>
#include <stdlib.h>

// To allow scan time messages in '()'
#define RM_ALLOW_LEXER_MESSAGES
// To allow parse time messages in '{}'
#define RM_ALLOW_PARSE_MESSAGES

typedef enum{
    #define ET(x) TOKEN_##x
    ET(comma),
    ET(register),
    ET(hash),
    ET(address),
    ET(number),
    ET(eof),
    ET(unknown),
    ET(labelDefine),
    ET(labelAccess),
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
    uint8_t length;
    size_t line;
    TokenType type;
} Token;

typedef struct{
    Token *tokens;
    uint32_t count;
    uint32_t hasError;
} TokenList;

TokenList tokens_scan(const char *source);
void tokens_free(TokenList list);
