#pragma once

#include "lexer.h"
#include <stdint.h>
#include <stdbool.h>

bool parse_and_emit(TokenList list, char *source, uint8_t *memory, uint32_t memSize, uint32_t offset);
