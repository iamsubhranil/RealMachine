#include "parser.h"
#include "bytecode.h"
#include "vm.h"
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

static uint32_t present = 0, length = 0, presentOffset = 0, memSize = 0, hasErrors = 0;
static uint8_t *memory;

static void writeByte(uint8_t byte){
    if(presentOffset >= memSize){
        memory = (uint8_t *)realloc(memory, sizeof(uint8_t) * (presentOffset + 1));
        memSize++;
    }
    if(!hasErrors){
        bc_write_byte(memory, &presentOffset, byte);
    }
    else
        bc_write_byte(memory, &presentOffset, OP_nex);
}

static void writeLong(uint32_t l){
    for(uint32_t i = 24;1;i -= 8){
        writeByte(l >> i);
        if(i == 0)
            break;
    }
}
/* Label system with forward referencing
 * =====================================
 */

typedef struct{
    Token label;
    uint8_t isInit;
    uint32_t *references;
    uint32_t refCount;
    uint32_t offset;
} Label;

static Label *labels = NULL;
static uint32_t labelCount = 0;

static void newLabel(uint32_t declOffset, Token t, uint8_t isInit){
    labels = (Label *)realloc(labels, sizeof(Label) * ++labelCount);
    labels[labelCount - 1].label = t;
    labels[labelCount - 1].offset = declOffset;
    labels[labelCount - 1].references = NULL;
    labels[labelCount - 1].refCount = 0;
    labels[labelCount - 1].isInit = isInit;
}

static void declareLabel(Token t, uint32_t declOffset){
    for(uint32_t i = 0;i < labelCount;i++){
        if(strcmp(labels[i].label.string, t.string) == 0){
            if(labels[i].isInit == 0){
                labels[i].offset = declOffset;
                labels[i].isInit = 1;
                labels[i].label = t;
            }
            else{
                err("Label '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "' is already defined!", t.string);
                token_print_source(labels[i].label, 1);
                hasErrors++;
            }
            return;
        }
    }
    newLabel(declOffset, t, 1);
}

static void addRef(Label *label, uint32_t ref){
    label->references = (uint32_t *)realloc(label->references, sizeof(uint32_t) * ++label->refCount);
    label->references[label->refCount - 1] = ref;
}

static void addReference(Token label, uint32_t reference){
    for(uint32_t i = 0;i < labelCount;i++){
        if(strcmp(labels[i].label.string, label.string) == 0){
            addRef(&labels[i], reference);
            return;
        }
    }
    newLabel(0, label, 0);
    addRef(&labels[labelCount - 1], reference);
}

static void checkLabels(){
    for(uint32_t i = 0;i < labelCount;i++){
        if(labels[i].isInit == 0){
            err("Label '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "' used but not defined!",
                    labels[i].label.string);
                token_print_source(labels[i].label, 1);
            hasErrors++;
        }
        else if(labels[i].refCount == 0){
            warn("Label '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "' defined but not used!",
                    labels[i].label.string);
                token_print_source(labels[i].label, 2);
        }
        else{
            uint32_t bak = presentOffset;
            for(uint32_t j = 0;j < labels[i].refCount;j++){
                presentOffset = labels[i].references[j];
                writeLong(labels[i].offset);
            }
            presentOffset = bak;
        }
        free(labels[i].references);
    }
    free(labels);
    labels = NULL;
    labelCount = 0;
}

// ================================================

/* Parser core
 * ===========
 */
static TokenList list;
static size_t presentLine = 0;
static Token presentToken, previousToken;

static uint8_t ueofShown = 0;

static void advance(){
    if(present < length){
        previousToken = presentToken;
        present++;
        presentToken = list.tokens[present];
        presentLine = presentToken.line;
    }
    else{
        if(!ueofShown){
            err("Unexpected end of file!");
            token_print_source(presentToken, 1);
            ueofShown = 1;
            hasErrors++;
        }
    }
}

bool match(TokenType type){
    if(list.tokens[present].type == type){
        return true;
    }
    return false;
}

static const char* tokenStrings[] = { 
    #define ET(x) #x,
    #include "tokens.h"
    #undef ET
};

bool consume(TokenType type){
    if(match(type)){
        advance();
        return true;
    }
    else if(presentToken.type == TOKEN_eof){
        if(!ueofShown){
            err("Unexpected end of file!");
            token_print_source(presentToken, 1);
            ueofShown = 1;
            hasErrors++;
        }
        return false;
    }
    else{
        err("Unexpected token : '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET
                "', Expected : '" ANSI_FONT_BOLD "%s" ANSI_COLOR_RESET "'", presentToken.string,
                tokenStrings[type]);
        token_print_source(presentToken, 1);
        advance();
        hasErrors++;
        return false;
    }
}

static void reg(){
    consume(TOKEN_register);
    if(consume(TOKEN_number)){
        char *end;
        uint64_t num = strtoll(previousToken.string, &end, 10);
        if(num > 7){
            err("Register number must be < 8, received " ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%" PRIu64 ANSI_COLOR_RESET, num);
            token_print_source(previousToken, 1);
            hasErrors++;
            writeByte(0);
        }
        else
            writeByte(num);
    }
}

static void ch(){
    if(consume(TOKEN_aphostrophy) && consume(TOKEN_label)){
        if(strlen(previousToken.string) > 1){
            warn("More than one character quoted!");
            token_print_source(previousToken, 2);
        }
        char c = previousToken.string[0];
        writeByte((int)c);
        consume(TOKEN_aphostrophy);
    }
}

static bool num(){
    if(consume(TOKEN_number)){
        char *end;
        uint64_t num = strtoll(previousToken.string, &end, 10);
        if(num > UINT32_MAX){
            err("Long constant must be " ANSI_FONT_BOLD "<= %" PRIu32 ANSI_COLOR_RESET
                    ", received " ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%" PRIu64 ANSI_COLOR_RESET, 
                                                         UINT32_MAX, num);
            token_print_source(previousToken, 1);
            hasErrors++;
            writeLong(0);
        }
        else
            writeLong(num);
        return true;
    }
    else
        writeLong(0);
    return false;
}

static void ref(){
    consume(TOKEN_address);
    if(match(TOKEN_label)){
        addReference(presentToken, presentOffset);
        writeLong(0);
        consume(TOKEN_label);
    }
    else
        num();
}

static void imm(){
    consume(TOKEN_hash);
    num();
}

#define TWOREG(name) \
    static void statement_##name(){ \
        writeByte(OP_##name); \
        reg(); \
        consume(TOKEN_comma); \
        reg(); \
    }

TWOREG(add)

TWOREG(sub)

TWOREG(mul)

TWOREG(div)

TWOREG(and)

TWOREG(or)

static void statement_not(){
    writeByte(OP_not);
    reg();
}

static void statement_lshift(){
    writeByte(OP_lshift);
    reg();
    consume(TOKEN_comma);
    imm();
}

static void statement_rshift(){
    writeByte(OP_rshift);
    reg();
    consume(TOKEN_comma);
    imm();
}

static void statement_load(){
    writeByte(OP_load);
    ref();
    consume(TOKEN_comma);
    reg();
}

static void statement_store(){
    writeByte(OP_store);
    reg();
    consume(TOKEN_comma);
    ref();
}

static void statement_mov(){
    writeByte(OP_mov);
    imm();
    consume(TOKEN_comma);
    reg();
}

static void statement_save(){
    writeByte(OP_save);
    imm();
    consume(TOKEN_comma);
    ref();
}

static void statement_print(){
    writeByte(OP_print);
    ref();
}

static void statement_printc(){
    writeByte(OP_printc);
    ref();
}

void statement_mcopy(){
    writeByte(OP_mcopy);
    ref();
    consume(TOKEN_comma);
    ref();
}

void statement_rcopy(){
    writeByte(OP_rcopy);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_jmp(){
    writeByte(OP_jmp);
    ref();
}

#define parseJump(x) \
static void statement_##x(){ \
    writeByte(OP_##x); \
    reg(); \
    consume(TOKEN_comma); \
    reg(); \
    consume(TOKEN_comma); \
    ref(); \
}

parseJump(jeq)

parseJump(jne)

parseJump(jgt)

parseJump(jlt)

parseJump(jov)

parseJump(jun)

#define parseNoop(x) \
static void statement_##x(){ \
    writeByte(OP_##x); \
}

parseNoop(clrsr)

parseNoop(clrpc)

parseNoop(halt)

parseNoop(nex)

static void statement_const(){
    imm();
}

static void statement_char(){
    ch();
}

void statement_label(){
    Token label = presentToken;
    advance();
    if(consume(TOKEN_colon)){
        declareLabel(label, presentOffset);
    }
}

#ifdef RM_ALLOW_PARSE_MESSAGES
void statement_parseMessage(){
    printf("%s", presentToken.string);
    advance();
}
#endif

bool parse_and_emit(TokenList l, uint8_t **mem, uint32_t *memS, uint32_t offset){
    memory = *mem;
    memSize = *memS;
    presentOffset = offset;
    presentToken = l.tokens[0];
    presentLine = presentToken.line;
    list = l;
    length = list.count;
    ueofShown = 0;

    while(!match(TOKEN_eof)){
        switch(presentToken.type){
            #define OPCODE(name, a, b) \
                case TOKEN_##name: \
                        consume(TOKEN_##name); \
                        statement_##name(); \
                        break;
            #include "opcodes.h"
            #undef OPCODE
                case TOKEN_label:
                    statement_label();
                    break;
#ifdef RM_ALLOW_PARSE_MESSAGES
                case TOKEN_parseMessage:
                    statement_parseMessage();
                    break;
#endif
                default:
                    err("Bad token : '" ANSI_FONT_BOLD ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "'", presentToken.string);
                    token_print_source(presentToken, 1);
                    hasErrors++;
                    advance();
                    break;
        }
    }
    *mem = memory;
    *memS = memSize;
    checkLabels();
    if(hasErrors){
        err("Compilation failed with " ANSI_FONT_BOLD  ANSI_COLOR_RED "%" PRIu32 ANSI_COLOR_RESET " errors!", hasErrors);
        return false;
    }
    return true;
}
