#include "parser.h"
#include "bytecode.h"
#include "vm.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

static uint32_t present = 0, length = 0, presentOffset = 0, memSize = 0, hasErrors = 0;
static uint8_t *memory;

static void writeByte(uint8_t byte){
    if(!hasErrors)
        bc_write_byte(memory, &presentOffset, byte);
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
    char *label;
    uint8_t isInit;
    uint32_t *references;
    uint32_t refCount;
    uint32_t offset;
    size_t line;
} Label;

static Label *labels = NULL;
static uint32_t labelCount = 0;

static void newLabel(char *name, uint32_t declOffset, size_t line, uint8_t isInit){
    labels = (Label *)realloc(labels, sizeof(Label) * ++labelCount);
    labels[labelCount - 1].label = name;
    labels[labelCount - 1].offset = declOffset;
    labels[labelCount - 1].references = NULL;
    labels[labelCount - 1].refCount = 0;
    labels[labelCount - 1].line = line;
    labels[labelCount - 1].isInit = isInit;
}

static void declareLabel(Token t, uint32_t declOffset){
    for(uint32_t i = 0;i < labelCount;i++){
        if(strcmp(labels[i].label, t.string) == 0){
            if(labels[i].isInit == 0){
                labels[i].offset = declOffset;
                labels[i].isInit = 1;
                labels[i].line = t.line;
            }
            else{
                printf("\n[Compilation error][line %zd] Label '%s' is already defined!", t.line, t.string);
                hasErrors++;
            }
            return;
        }
    }
    newLabel(t.string, declOffset, t.line, 1);
}

static void addRef(Label *label, uint32_t ref){
    label->references = (uint32_t *)realloc(label->references, sizeof(uint32_t) * ++label->refCount);
    label->references[label->refCount - 1] = ref;
}

static void addReference(Token label, uint32_t reference){
    for(uint32_t i = 0;i < labelCount;i++){
        if(strcmp(labels[i].label, label.string) == 0){
            addRef(&labels[i], reference);
            return;
        }
    }
    newLabel(label.string, 0, label.line, 0);
    addRef(&labels[labelCount - 1], reference);
}

static void checkLabels(){
    for(uint32_t i = 0;i < labelCount;i++){
        if(labels[i].isInit == 0){
            printf("\n[Compilation error][line %zd] Label '%s' used but not defined!", labels[i].line, labels[i].label);
            hasErrors++;
        }
        else if(labels[i].refCount == 0){
            printf("\n[Compilation warning][line %zd] Label '%s' defined but not used!", labels[i].line, labels[i].label);
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

static void advance(){
    if(present < length){
        previousToken = presentToken;
        present++;
        presentToken = list.tokens[present];
        presentLine = presentToken.line;
    }
    else{
        printf("\n[Compilation error] Unexpected end of file!");
        hasErrors++;
    }
}

bool match(TokenType type){
    if(list.tokens[present].type == type){
        return true;
    }
    return false;
}

#define ET(x) #x
static const char* tokenStrings[] = { 
    ET(comma),
    ET(register),
    ET(hash),
    ET(address),
    ET(number),
    ET(eof),
    ET(unknown),
    ET(label),
    ET(colon),
#ifdef RM_ALLOW_PARSE_MESSAGES
    ET(parseMessage),
#endif
    #define OPCODE(x, a, b) ET(x),
    #include "opcodes.h"
    #undef OPCODE
};
#undef ET

bool consume(TokenType type){
    if(match(type)){
        advance();
        return true;
    }
    else{
        printf("\n[Compilation error][line %zd] Unexpected token : '%s', Expected : '%s'", presentLine, presentToken.string,
                tokenStrings[type]);
        if(presentToken.type != TOKEN_eof)
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
            printf("\n[Compilation error][line %zd] Register number must be < 8, received %" PRIu64, presentLine, num);
            hasErrors++;
            writeByte(0);
        }
        else
            writeByte(num);
    }
}

static bool num(){
    if(consume(TOKEN_number)){
        char *end;
        uint64_t num = strtoll(previousToken.string, &end, 10);
        if(num > UINT32_MAX){
            printf("\n[Compilation error][line %zd] Memory reference must be < %" PRIu32 ", received %" PRIu64, 
                                                        previousToken.line, UINT32_MAX, num);
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

static void statement_add(){
    writeByte(OP_add);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_sub(){
    writeByte(OP_sub);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_mul(){
    writeByte(OP_mul);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_div(){
    writeByte(OP_div);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_and(){
    writeByte(OP_and);
    reg();
    consume(TOKEN_comma);
    reg();
}

static void statement_or(){
    writeByte(OP_or);
    reg();
    consume(TOKEN_comma);
    reg();
}

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

bool parse_and_emit(TokenList l, uint8_t *mem, uint32_t memS, uint32_t offset){
    memory = mem;
    memSize = memS;
    presentOffset = offset;
    presentToken = l.tokens[0];
    presentLine = presentToken.line;
    list = l;
    length = list.count;
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
                    printf("\n[Compilation error][line %zd] Bad token : '%s'", presentLine, presentToken.string);
                    hasErrors++;
                    advance();
                    break;
        }
    }
    checkLabels();
    if(hasErrors){
        printf("\n[Compilation error] Compilation failed with %" PRIu32 " errors!", hasErrors);
        return false;
    }
    return true;
}