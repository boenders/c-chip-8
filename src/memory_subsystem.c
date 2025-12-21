#include "memory_subsystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Memory Layout
//
// 0x00 INSTRUCTION_POINTER
// 0x02 INDEX_REGISTER
// 0x04 DELAY_TIMER
// 0x05 SOUND_TIMER
// 0x06 STACK_POSITION
//
// 0x07 EMPTY
//
// 0x10 REGISTER
//
// 0x20 EMPTY
//
// 0x50 FONT
// 0x100 STACK
//
// 0x140 EMPTY
//
// 0x200 Program_Start

#define INSTRUCTION_POINTER 0x00
#define INDEX_REGISTER 0x02
#define DELAY_TIMER 0x04
#define SOUND_TIMER 0x05
#define STACK_LENGTH 0x06
#define REGISTER 0x10
#define FONT_POSITION 0x50
#define STACK_POSITION 0x100

#define FONT                                                                   \
    {0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,   \
     0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,   \
     0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,   \
     0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,   \
     0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,   \
     0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,   \
     0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80}
#define STACK_MAX 0x20 // Stack reaches until 0x140

memory_subsystem *memory_init(void) {
    memory_subsystem *m = malloc(sizeof *m);
    uint8_t font[] = FONT;
    memcpy(m->memory + FONT_POSITION, font, sizeof(font));

    uint16_t *instruction = (uint16_t *)(m->memory + INSTRUCTION_POINTER);
    *instruction = PROGRAM_START;
    return m;
}

void memory_free(memory_subsystem *mem) {
    free(mem);
    mem = NULL;
}

uint16_t memory_get_instruction(memory_subsystem *mem) {
    // No little-endian swap required here as the value written was already
    // written as a little-endian
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    // Flip to correctly handle little-endian.
    uint16_t result = *(uint8_t *)(mem->memory + *instruction) << 8;
    result += *(uint8_t *)(mem->memory + *instruction + 1);
    *instruction += 2;

    return result;
}
void memory_set_instruction(memory_subsystem *mem, uint16_t value) {
    // No little-endian swap here as the rotation is helpful during access.
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    *instruction = value;
}
void memory_skip_instruction(memory_subsystem *mem) {
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    (*instruction) += 2;
}
void memory_repeat_instruction(memory_subsystem *mem) {
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    (*instruction) -= 2;
}
void memory_set_register(memory_subsystem *mem, uint8_t register_no,
                         uint8_t value) {
    *(mem->memory + REGISTER + register_no) = value;
}
uint8_t memory_get_register(memory_subsystem *mem, uint8_t register_no) {
    return *(mem->memory + REGISTER + register_no);
}
void memory_add_to_register(memory_subsystem *mem, uint8_t register_no,
                            uint8_t value) {
    *(mem->memory + REGISTER + register_no) += value;
}
uint16_t memory_get_index_register(memory_subsystem *mem) {
    uint16_t *index_register = (uint16_t *)(mem->memory + INDEX_REGISTER);
    return *index_register;
}
void memory_set_index_register(memory_subsystem *mem, uint16_t address) {
    uint16_t *index_register = (uint16_t *)(mem->memory + INDEX_REGISTER);
    *index_register = address;
}
uint8_t *memory_get_sprite(memory_subsystem *mem) {
    uint16_t *index_register = (uint16_t *)(mem->memory + INDEX_REGISTER);
    return mem->memory + *index_register;
}
void memory_instruction_jump_to(memory_subsystem *mem, uint16_t value) {
    // To ignore endian issues the bits from the instruction are directly
    // memcopied over to the stack.
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    uint8_t *stack_position = mem->memory + STACK_POSITION;
    memcpy(mem->memory + STACK_POSITION + sizeof(uint16_t) * *stack_position,
           instruction, sizeof(uint16_t));
    *stack_position += 1;
    *instruction = value;
}
void memory_instruction_jump_back(memory_subsystem *mem) {
    // To ignore endian issues the bits from the stack are directly
    // memcopied back to the instruction register.
    uint16_t *instruction = (uint16_t *)(mem->memory + INSTRUCTION_POINTER);
    uint8_t *stack_position = mem->memory + STACK_POSITION;
    *stack_position -= 1;
    memcpy(instruction,
           mem->memory + STACK_POSITION + sizeof(uint16_t) * *stack_position,
           sizeof(uint16_t));
}
void memory_store_registers(memory_subsystem *mem, uint8_t count,
                            bool increment) {
    uint16_t *index_register = (uint16_t *)(mem->memory + INDEX_REGISTER);
    for (int i = 0; i <= count; i++) {
        *(mem->memory + i + *index_register) = memory_get_register(mem, i);
    }
    if (increment) {
        *index_register += count + 1;
    }
}
void memory_load_registers(memory_subsystem *mem, uint8_t count,
                           bool increment) {
    uint16_t *index_register = (uint16_t *)(mem->memory + INDEX_REGISTER);
    for (int i = 0; i <= count; i++) {
        memory_set_register(mem, i, *(mem->memory + i + *index_register));
    }
    if (increment) {
        *index_register += count + 1;
    }
}
uint8_t memory_get_delay_timer(memory_subsystem *mem) {
    return *(mem->memory + DELAY_TIMER);
}
void memory_set_delay_timer(memory_subsystem *mem, uint8_t value) {
    *(mem->memory + DELAY_TIMER) = value;
}
uint8_t memory_get_sound_timer(memory_subsystem *mem) {
    return *(mem->memory + SOUND_TIMER);
}
void memory_set_sound_timer(memory_subsystem *mem, uint8_t value) {
    *(mem->memory + SOUND_TIMER) = value;
}
