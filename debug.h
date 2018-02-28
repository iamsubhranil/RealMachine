#pragma once

#include "vm.h"
#include <stdint.h>

void debugRegister(VirtualMachine *machine, uint8_t index);
void debugInstruction(uint8_t *memory, uint32_t *offset);
