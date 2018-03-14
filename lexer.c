#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <inttypes.h>

#include "lexer.h"
#include "display.h"

static Token keywords[] = {
    #define ET(x) TOKEN_##x
    #define OPCODE(name, a, length) {#name, NULL, length, 0, 0, 0, ET(name)},
    #include "opcodes.h"
    #undef OPCODE
    #undef ET
};

static char* source = NULL;
static size_t present = 0, length = 0, start = 0, line = 1;
static Token lastToken;

static void addToken(TokenList *list, Token token){
    list->tokens = (Token *)realloc(list->tokens, sizeof(Token) * (++list->count));
    token.source = list->source;
    list->tokens[list->count - 1] = token;
}

static Token makeToken(TokenType type){
    Token t;
    t.source = NULL;
    t.type = type;
    t.line = line;
    t.string = (char *)malloc(sizeof(char) * (present - start + 1));
    strncpy(t.string, &(source[start]), present-start);
    t.string[present - start] = '\0';
    t.length = present - start;
    t.start = start;
    t.end = present - 1;
    lastToken = t;
    return t;
}

// rtype 1 --> error
// rtype 2 --> warning
// rtype 0 --> info
static void print_source(const char *s, size_t line, size_t hfrom, size_t hto, uint8_t rtype){
    if(rtype == 1)
        pred( ANSI_FONT_BOLD "\n<line %zd>\t" ANSI_COLOR_RESET, line);
    else if(rtype == 2)
        pylw( ANSI_FONT_BOLD "\n<line %zd>\t" ANSI_COLOR_RESET, line);
    else
        pblue( ANSI_FONT_BOLD "\n<line %zd>\t" ANSI_COLOR_RESET, line);
    
    if(s == NULL){
        pmgn("<source not available>");
        return;
    }
    size_t l = 1, p = 0;
    while(l < line){
        if(s[p] == '\n')
            l++;
        p++;
    }
    for(size_t i = p;s[i] != '\n' && s[i] != '\0';i++)
        printf("%c", s[i]);
    printf("\n            \t");
    for(size_t i = p;i <= hto;i++){
        if(i >= hfrom && i <= hto)
            pmgn(ANSI_FONT_BOLD "~" ANSI_COLOR_RESET);
        else
            printf(" ");
    }
    printf("\n");
}

void token_print_source(Token t, uint8_t rtype){
    print_source(t.source, t.line, t.start, t.end, rtype);
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

    // It may be a label, leave it to the parser
    
    return makeToken(TOKEN_label);
}

static Token makeNumber(){
    if(source[present] == '-')
        present++;
    uint32_t bak = present;
    while(isdigit(source[present]))
        present++;
    if(present == bak){
        return makeToken(TOKEN_unknown);
    }
    return makeToken(TOKEN_number);
}

static Token nextToken(){
    start = present;
    if(present == length)
        return makeToken(TOKEN_eof);
    if(isalpha(source[present])){
        return makeKeyword();
    }
    else if(isdigit(source[present]) || source[present] == '-'){
        return makeNumber();
    }
    switch(source[present]){
        case ' ':
        case '\t':
        case '\r':
            present++;
            return nextToken();
        case '\n':
            present++;
            line++;
            return nextToken();
        case '@':
            present++;
            return makeToken(TOKEN_address);
        case ':':
            present++;
            return makeToken(TOKEN_colon);
        case '#':
            present++;
            return makeToken(TOKEN_hash);
        case ',':
            present++;
            return makeToken(TOKEN_comma);
        case '"':
            present++;
            start = present;
            while(source[present] != '"' && source[present] != '\0'){
                if(source[present] == '\\' && source[present+1] == '"')
                    present++;
                else if(source[present] == '\n')
                    line++;
                present++;
            }
            Token t = makeToken(TOKEN_string);
            if(source[present] == '"')
                present++;
            return t;
        case '[':
            present++;
            while(present < length && source[present] != ']'){
                if(source[present] == '\n')
                    line++;
                present++;
            }
            if(present < length && source[present] == ']')
                present++;
            return nextToken();
#ifdef RM_ALLOW_LEXER_MESSAGES
        case '(':
            present++;
            while(present < length && source[present] != ')'){
                if(source[present] == '\n')
                    line++;
                printf("%c", source[present]);
                present++;
            }
            fflush(stdin);
            if(present < length && source[present] == ')')
                present++;
            return nextToken();
#endif
#ifdef RM_ALLOW_PARSE_MESSAGES
        case '{':
            present++;
            start = present;
            while(present < length && source[present] != '}'){ 
                if(source[present] == '\n')
                    line++;
                present++;
            }
            if(present < length && source[present] == '}')
                source[present] = ' ';
            return makeToken(TOKEN_parseMessage);
#endif
    }
    present++;
    return makeToken(TOKEN_unknown);
}

TokenList tokens_scan(const char* line){
    source = strdup(line);
    length = strlen(source);
    present = 0;
    start = 0;

    TokenList list = {source, NULL, 0, 0};
    while(present < length){
        Token t = nextToken();
        list.hasError += t.type == TOKEN_unknown;
        addToken(&list, t);
        if(t.type == TOKEN_unknown){ 
            err("Unexpected character '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "'!", t.string);
            token_print_source(list.tokens[list.count - 1], 1);
        }
    }
    if(list.tokens[list.count - 1].type != TOKEN_eof)
        addToken(&list, makeToken(TOKEN_eof));
    return list;
}


void tokens_free(TokenList list){
    for(uint32_t i = 0;i < list.count;i++)
        free(list.tokens[i].string);
    free(list.tokens);
    free(list.source);
}

/* The code below is for testing and debugging purposes.
 * =====================================================
 */

#ifdef DEBUG

static const char* tokenStrings[] = { 
    #define ET(x) #x,
    #include "tokens.h"
    #undef ET
};

static void printToken(Token t){
    pblue(" %s", tokenStrings[t.type]);
    pylw("(%s) \t", t.string);
}

void lexer_print_tokens(TokenList list){
    size_t prevLine = 0;
    for(uint32_t i = 0;i < list.count;i++){
        if(prevLine != list.tokens[i].line){
            pmgn(ANSI_FONT_BOLD "\n<line %zd>", list.tokens[i].line);
            prevLine = list.tokens[i].line;
        }
        printToken(list.tokens[i]);
    }
}

#endif
