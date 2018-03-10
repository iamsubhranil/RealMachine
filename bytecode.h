#pragma once

#include "rm_common.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct{
    uint8_t *memory;
    uint32_t size;
} Data;

void bc_write_byte(uint8_t *memory, uint32_t *offset, uint8_t data);
void bc_copy_arr(uint8_t *memory, uint8_t *data, uint32_t size, uint32_t offset);
Data bc_read_from_disk(const char *fileName);
bool bc_save_to_disk(const char *fileName, uint8_t *memory, uint32_t size);
void bc_write_op(uint8_t *memory, uint32_t *offset, int opcode, ...);
