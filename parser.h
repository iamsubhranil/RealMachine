#pragma once
#include "rm_common.h"
#include "lexer.h"
#include <stdint.h>
#include <stdbool.h>

bool parse_and_emit(TokenList list, uint8_t **memory, uint32_t *memSize, uint32_t offset);
