#ifndef MEMORY_SUBSYSTEM
#define MEMORY_SUBSYSTEM

#include <stdint.h>
#define MEMORY 4096
#define VARIABLES 16
#define PROGRAM_START 0x200

typedef struct {
    uint8_t memory[MEMORY];
    uint16_t program_counter;
    uint16_t index_register;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t stack_position;
    uint8_t registers[VARIABLES];
} memory_subsystem;

memory_subsystem *memory_init(void);
void memory_free(memory_subsystem *mem);
uint16_t get_instruction(memory_subsystem *mem);
void set_instruction(memory_subsystem *mem, uint16_t value);
void skip_instruction(memory_subsystem *mem);
void repeat_instruction(memory_subsystem *mem);
void set_register(memory_subsystem *mem, uint8_t register_no, uint8_t value);
uint8_t get_register(memory_subsystem *mem, uint8_t register_no);
void add_to_register(memory_subsystem *mem, uint8_t register_no, uint8_t value);
void set_index_register(memory_subsystem *mem, uint16_t address);
uint8_t *get_sprite(memory_subsystem *mem);
void jump_to(memory_subsystem *mem, uint16_t value);
void jump_back(memory_subsystem *mem);
void store_registers(memory_subsystem *mem, uint8_t count);
void load_registers(memory_subsystem *mem, uint8_t count);

#endif // !MEMORY_SUBSYSTEM
