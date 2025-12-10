#include "memory_subsystem.h"
#include <stdlib.h>
#include <string.h>

#define FONT                                                                   \
    {0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,   \
     0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,   \
     0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,   \
     0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,   \
     0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,   \
     0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,   \
     0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80}
#define STACK 0x100
#define STACK_MAX 0x20

memory_subsystem *memory_init(void) {
    memory_subsystem *m = malloc(sizeof *m);
    uint8_t font[] = FONT;
    memcpy(m->memory + 0x50, font, sizeof(font));

    m->program_counter = PROGRAM_START;
    return m;
}

void memory_free(memory_subsystem *mem) { free(mem); }

uint16_t get_instruction(memory_subsystem *mem) {
    uint16_t result = (mem->memory[mem->program_counter++] << 8);
    result = result | mem->memory[mem->program_counter++];
    return result;
}
void set_instruction(memory_subsystem *mem, uint16_t value) {
    mem->program_counter = value;
}
void skip_instruction(memory_subsystem *mem) { mem->program_counter += 2; }
void repeat_instruction(memory_subsystem *mem) { mem->program_counter -= 2; }
void set_register(memory_subsystem *mem, uint8_t register_no, uint8_t value) {
    mem->registers[register_no] = value;
}
uint8_t get_register(memory_subsystem *mem, uint8_t register_no) {
    return mem->registers[register_no];
}
void add_to_register(memory_subsystem *mem, uint8_t register_no,
                     uint8_t value) {
    mem->registers[register_no] += value;
}
void set_index_register(memory_subsystem *mem, uint16_t address) {
    mem->index_register = address;
}
uint8_t *get_sprite(memory_subsystem *mem) {
    return mem->memory + mem->index_register;
}
void jump_to(memory_subsystem *mem, uint16_t value) {
    memcpy(mem->memory + STACK + sizeof(uint16_t) * mem->stack_position++,
           &mem->program_counter, sizeof(uint16_t));
    mem->program_counter = value;
}
void jump_back(memory_subsystem *mem) {
    memcpy(&mem->program_counter,
           mem->memory + STACK + sizeof(uint16_t) * --mem->stack_position,
           sizeof(uint16_t));
}
void store_registers(memory_subsystem *mem, uint8_t count) {
    for (int i = 0; i <= count; i++) {
        *(mem->memory + mem->index_register + i) = get_register(mem, i);
    }
}
void load_registers(memory_subsystem *mem, uint8_t count) {
    for (int i = 0; i <= count; i++) {
        set_register(mem, i, *(mem->memory + mem->index_register + i));
    }
}
