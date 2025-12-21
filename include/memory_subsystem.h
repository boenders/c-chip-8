#ifndef MEMORY_SUBSYSTEM
#define MEMORY_SUBSYSTEM

#include <stdbool.h>
#include <stdint.h>
#define MEMORY 4096
#define VARIABLES 16
#define PROGRAM_START 0x200

typedef struct {
    uint8_t memory[MEMORY];
    uint16_t index_register;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t stack_position;
    uint8_t registers[VARIABLES];
} memory_subsystem;

/**
 * Allocates a new memory_subsystem struct on heap memory.
 *
 * The initialization also loads the font to use into memory.
 *
 * The struct must be freed using the "memory_free" function.
 */
memory_subsystem *memory_init(void);
/**
 * Frees all memory allocated for the memory subsystem.
 *
 * The pointer provided to this function must not be used after calling it.
 */
void memory_free(memory_subsystem *mem);
/**
 * Returns the instruction that the program currently points to.
 * 
 * This will also advance the instruction pointer by two bytes to point
 * to the next instruction.
 */
uint16_t memory_get_instruction(memory_subsystem *mem);
/**
 * Sets the instruction pointer to a new location in memory.
 */
void memory_set_instruction(memory_subsystem *mem, uint16_t value);
/**
 * Moves the instruction pointer two bytes forwards to skip to the next
 * instruction.
 */
void memory_skip_instruction(memory_subsystem *mem);
/**
 * Moves the instruction pointer two bytes back to repeat the last instruction.
 *
 * Used in conjunction with memory_get_instruction this will lead to the
 * instruction pointer not changing its position.
 */
void memory_repeat_instruction(memory_subsystem *mem);
/**
 * Set one of the variable registers to a new value.
 */
void memory_set_register(memory_subsystem *mem, uint8_t register_no, uint8_t value);
/**
 * Get a value from a variable register.
 */
uint8_t memory_get_register(memory_subsystem *mem, uint8_t register_no);
/**
 * Get the address that the index register currently points at.
 */
uint16_t memory_get_index_register(memory_subsystem *mem);
/**
 * Set index register to point to a new location in memory.
 */
void memory_set_index_register(memory_subsystem *mem, uint16_t address);
/**
 * Get pointer to sprite from the location in memory pointed to by the index register.
 */
uint8_t *memory_get_sprite(memory_subsystem *mem);
/**
 * Sets the instruction pointer to a new location in memory and stores the
 * current instruction on the stack for later reuse.
 *
 * The stack has a limited size of 32 instructions.
 */
void memory_instruction_jump_to(memory_subsystem *mem, uint16_t value);
/**
 * Sets the instruction pointer to the last location on the stack and pops 
 * the value from the stack.
 */
void memory_instruction_jump_back(memory_subsystem *mem);
/**
 * Stores count registers starting from the first at the location of the
 * index_register.
 *
 * From register 0 -> count
 * into
 * index_register -> index_register + count
 *
 * Setting the increment flag will increment the index register pointer.
 */
void memory_store_registers(memory_subsystem *mem, uint8_t count, bool increment);
/**
 * Reads count registers starting from the location of the index_register and
 * writes them back into the variable registers.
 *
 * index_register -> index_register + count
 * into
 * 0 -> count
 *
 * Setting the increment flag will increment the index register pointer.
 */
void memory_load_registers(memory_subsystem *mem, uint8_t count, bool increment);
#endif // !MEMORY_SUBSYSTEM
