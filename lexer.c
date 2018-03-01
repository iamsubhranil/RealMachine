#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <inttypes.h>
#include "lexer.h"

static Token keywords[] = {
    #define ET(x) TOKEN_##x
    #define OPCODE(name, a, length) {#name, length, 0, ET(name)},
    #include "opcodes.h"
    #undef OPCODE
    #undef ET
};

static char* source = NULL;
static size_t present = 0, length = 0, start = 0, line = 1;
static Token lastToken;

static void addToken(TokenList *list, Token token){
    list->tokens = (Token *)realloc(list->tokens, sizeof(Token) * (++list->count));
    list->tokens[list->count - 1] = token;
}

static Token makeToken(TokenType type){
    Token t;
    t.length = 1;
    t.type = type;
    t.line = line;
    t.string = (char *)malloc(sizeof(char) * (present - start + 1));
    strncpy(t.string, &(source[start]), present-start);
    t.string[present - start] = '\0';
    t.length = present - start;
    lastToken = t;
    return t;
}

static Token error(){
    Token m = makeToken(TOKEN_unknown);
    printf("\n[Error] Unexpected token '%s' at line %zd !", m.string, m.line);
    return m;
}

static Token makeKeyword(){
    while(isalpha(source[present]))
        present++;
    if(present - start == 1 && source[start] =='r')
        return makeToken(TOKEN_register);
    char word[present - start + 1];
    for(uint32_t i = start;i < present;i++){
        word[i - start] = source[i];
    };
    word[present - start] = '\0';
    for(uint32_t i = 0;i < sizeof(keywords)/sizeof(Token);i++){
        if(keywords[i].length == (present - start)){
            if(strcmp(keywords[i].string, word) == 0){
                return makeToken(keywords[i].type);
            }
        }
    }

    // Checks to see if this token might be a label
    
    // See if there is any '@' previously in the same line
    if(line == lastToken.line && lastToken.type == TOKEN_address)
        return makeToken(TOKEN_labelAccess);

    // See if there is a ':' next
    size_t temp = present;
    while(source[temp] == ' ' || source[temp] == '\t')
        temp++;
    if(source[temp] == ':'){
        temp++;
        present = temp;
        return makeToken(TOKEN_labelDefine);
    }

    // Nope
    return error();
}

static Token makeNumber(){
    while(isdigit(source[present]))
        present++;
    return makeToken(TOKEN_number);
}

static Token nextToken(){
    start = present;
    if(present == length)
        return makeToken(TOKEN_eof);
    if(isalpha(source[present])){
        return makeKeyword();
    }
    else if(isdigit(source[present])){
        return makeNumber();
    }
    switch(source[present]){
        case ' ':
        case '\t':
            present++;
            return nextToken();
        case '\n':
            present++;
            line++;
            return nextToken();
        case '@':
            present++;
            return makeToken(TOKEN_address);
        case '#':
            present++;
            return makeToken(TOKEN_hash);
        case ',':
            present++;
            return makeToken(TOKEN_comma);
        case '[':
            present++;
            while(present < length && source[present] != ']')
                present++;
            if(present < length && source[present] == ']')
                present++;
            return nextToken();
#ifdef RM_ALLOW_LEXER_MESSAGES
        case '(':
            present++;
            while(present < length && source[present] != ')'){
                printf("%c", source[present]);
                present++;
            }
            if(present < length && source[present] == ')')
                present++;
            return nextToken();
#endif
#ifdef RM_ALLOW_PARSE_MESSAGES
        case '{':
            present++;
            start = present;
            while(present < length && source[present] != '}')
                present++;
            if(present < length && source[present] == '}')
                source[present] = ' ';
            return makeToken(TOKEN_parseMessage);
#endif
    }
    present++;
    return error();
}

TokenList tokens_scan(const char* line){
    source = strdup(line);
    length = strlen(source);
    present = 0;
    start = 0;
    TokenList list = {NULL, 0, 0};
    while(present < length){
        Token t = nextToken();
        list.hasError += t.type == TOKEN_unknown;
        addToken(&list, t);
    }
    free(source);
    return list;
}


void tokens_free(TokenList list){
    for(uint32_t i = 0;i < list.count;i++)
        free(list.tokens[i].string);
    free(list.tokens);
}

/* The code below is for testing and debugging purposes.
 * =====================================================
 */

#define ET(x) #x
const char* tokenStrings[] = { 
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
};

static void printToken(Token t){
    printf("\n%11s(%10s)\tline : %2zd", tokenStrings[t.type], t.string, t.line);
}

static void printTokens(TokenList list){
    for(uint32_t i = 0;i < list.count;i++)
        printToken(list.tokens[i]);
    if(list.hasError)
        printf("\n[Warning] Scanning completed with %" PRIu32 " errors!", list.hasError);
}

static void test(){
    TokenList l = tokens_scan("mov #3242, r0"
                "\n(this is a scan time message) jmplabel :"
                "\nmov r0, r1, @jmplabel"
                "\n{this is a parse time message}"
                "\n"
                "\nstore r0, @ewe [ this is a comment ]"
                "\n[this"
                "\n is a multiline"
                "\ncomment]"
                "\nprint @322");
    printTokens(l);
    tokens_free(l);
    printf("\n");
}