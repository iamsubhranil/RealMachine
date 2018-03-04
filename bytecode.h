#pragma once

#include <stdint.h>
#include <stdarg.h>

void bc_write_byte(uint8_t *memory, uint32_t *offset, uint8_t data);
void bc_copy_arr(uint8_t *memory, uint8_t *data, uint32_t size, uint32_t offset);
void bc_write_op(uint8_t *memory, uint32_t *offset, int opcode, ...);
