#pragma once
#if defined(DEBUG) || defined(DEBUG_INSTRUCTIONS)

#include "rm_common.h"
#include "vm.h"
#include <stdint.h>

void debugRegister(VirtualMachine *machine, uint8_t index);
void debugInstruction(uint8_t *memory, uint32_t *offset, uint32_t size);
#endif
