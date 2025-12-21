#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "flags.h"
#include "memory_subsystem.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void decode(uint16_t instruction, memory_subsystem *mem, renderer *r,
            SDL_Scancode code);
uint8_t scancode_to_key(SDL_Scancode code);
uint8_t flags =
    VF_RESET | MEMORY_INDEX_INCREMENT | DISPLAY_WAIT | DISPLAY_CLIPPING;
// Flag used to slow sprite rendering to 60 sprites a second.
bool ready = true;

int main(int argc, char **argv) {
    printf("Got %i arguments", argc);
    if (argc < 2) {
        fprintf(stdout, "Arguments to specify: [arguments] gamefile\n\n");
        fprintf(stdout, "--disable-vf-reset  And, Or, Xor no longer affect the "
                        "flag register\n");
        fprintf(stdout, "--disable-memory-index-increment  Disable load/save "
                        "to affecting the index register\n");
        fprintf(stdout, "--disable-display-wait  Disables for the vertical "
                        "blank interrup\n");
        fprintf(stdout, "--disable-clipping  Allows sprites to overflow and be "
                        "drawn on the other side\n");
        fprintf(stdout,
                "--shifting-vx  Sets shifting to not take vy into account\n");
        fprintf(stdout, "--jumping-use-vx  Jumping uses vx instead of v0\n\n");
        fflush(stdout);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (!memcmp(*(argv + i), "--disable-vf-reset", strlen(*(argv + i)))) {
            flags ^= VF_RESET;
        } else if (!memcmp(*(argv + i), "--disable-memory-index-increment",
                           strlen(*(argv + i)))) {
            flags ^= MEMORY_INDEX_INCREMENT;
        } else if (!memcmp(*(argv + i), "--disable-display-wait",
                           strlen(*(argv + i)))) {
            flags ^= DISPLAY_WAIT;
        } else if (!memcmp(*(argv + i), "--disable-clipping",
                           strlen(*(argv + i)))) {
            flags ^= DISPLAY_CLIPPING;
        } else if (!memcmp(*(argv + i), "--shifting-vx", strlen(*(argv + i)))) {
            flags ^= SHIFT_USE_VX;
        } else if (!memcmp(*(argv + i), "--jumping-use-vx",
                           strlen(*(argv + i)))) {
            flags ^= JUMPING_USE_VX;
        }
    }
    char *filepath = *(argv + argc - 1);

    int rc = 0;
    FILE *fptr = fopen(filepath, "rb");
    if (!fptr) {
        printf("Could not open file: %s", *(argv + argc - 1));
        rc = 1;
        goto exit;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win =
        SDL_CreateWindow("Chip-8", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    renderer *r = renderer_init(ren);
    memory_subsystem *mem = memory_init();
    if (!r) {
        fprintf(stderr, "Renderer init error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_DestroyRenderer(ren);
        SDL_Quit();
        return 1;
    }

    if (!fread(mem->memory + PROGRAM_START, 1, MEMORY - PROGRAM_START, fptr)) {
        rc = 1;
        goto cleanup;
    }

    int old_width = 0;
    int old_height = 0;
    uint64_t timestamp = SDL_GetTicksNS();
    // SDL_KeyboardEvent *ke;
    SDL_Scancode code;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT ||
                e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = 0;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                code = (*(SDL_KeyboardEvent *)&e).scancode;
            }
            if (e.type == SDL_EVENT_KEY_UP) {
                code = 0;
            }
        }

        int current_width = 0;
        int current_height = 0;
        SDL_GetRenderOutputSize(ren, &current_width, &current_height);
        if (old_width != current_width || old_height != current_height) {
            old_width = current_width;
            old_height = current_height;
            renderer_lineup_pixels(r);
        }

        // Fetch
        uint16_t instruction = memory_get_instruction(mem);
        decode(instruction, mem, r, code);

        // Every 16ms -> 60 reductions a second
        if (16660000 < SDL_GetTicksNS() - timestamp) {
            timestamp = SDL_GetTicksNS();

            if (memory_get_delay_timer(mem) > 0) {
                memory_set_delay_timer(mem, memory_get_delay_timer(mem) - 1);
            }
            if (memory_get_sound_timer(mem) > 0) {
                memory_set_sound_timer(mem, memory_get_sound_timer(mem) - 1);
            }
            ready = true;
        }

        // 1.5ms
        SDL_DelayNS(1500000);
    }

cleanup:
    fclose(fptr);
    memory_free(mem);
    renderer_free(r);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
exit:
    return rc;
}

// Get the second nibble of an instruction.
#define getX(v) (v >> 8) & 0x000F
// Get the third nibble of an instruction.
#define getY(v) (v >> 4) & 0x000F
// Get the fourth nibble of an instruction.
#define getN(v) v & 0x000F
// Get the third and fourth nibble of an instruction.
#define getNN(v) v & 0x00FF
// Get the second, third and fourth nibble of an instruction.
#define getNNN(v) v & 0x0FFF

void decode(uint16_t instruction, memory_subsystem *mem, renderer *r,
            SDL_Scancode code) {
    uint16_t address;
    uint8_t value;
    uint8_t value_two;

    switch ((instruction >> 12) & 0x000F) {
    case 0x0:
        if (instruction == 0x00E0) {
            render_clear(r);
        } else if (instruction == 0x00EE) {
            memory_instruction_jump_back(mem);
        }
        break;
    case 0x1:
        address = getNNN(instruction);
        memory_set_instruction(mem, address);
        break;
    case 0x2:
        address = getNNN(instruction);
        memory_instruction_jump_to(mem, address);
        break;
    case 0x3:
        value = getNN(instruction);
        if (memory_get_register(mem, getX(instruction)) == value) {
            memory_skip_instruction(mem);
        }
        break;
    case 0x4:
        value = getNN(instruction);
        if (memory_get_register(mem, getX(instruction)) != value) {
            memory_skip_instruction(mem);
        }
        break;
    case 0x5:
        if (memory_get_register(mem, getX(instruction)) ==
            memory_get_register(mem, getY(instruction))) {
            memory_skip_instruction(mem);
        }
        break;
    case 0x6:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(mem, address, value);
        break;
    case 0x7:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(mem, address,
                            memory_get_register(mem, address) + value);
        break;
    case 0x8:
        address = getX(instruction);
        uint8_t x_value = memory_get_register(mem, getX(instruction));
        uint8_t y_value = memory_get_register(mem, getY(instruction));
        switch (getN(instruction)) {
        case 0x0:
            memory_set_register(mem, address, y_value);
            break;
        case 0x1:
            memory_set_register(mem, address, x_value | y_value);
            if (flags & VF_RESET) {
                memory_set_register(mem, 0xF, 0);
            }
            break;
        case 0x2:
            memory_set_register(mem, address, x_value & y_value);
            if (flags & VF_RESET) {
                memory_set_register(mem, 0xF, 0);
            }
            break;
        case 0x3:
            memory_set_register(mem, address, x_value ^ y_value);
            if (flags & VF_RESET) {
                memory_set_register(mem, 0xF, 0);
            }
            break;
        case 0x4:
            memory_set_register(mem, address, x_value + y_value);
            memory_set_register(mem, 0xF, x_value + y_value > UINT8_MAX);
            break;
        case 0x5:
            memory_set_register(mem, address, x_value - y_value);
            memory_set_register(mem, 0xF, x_value >= y_value);
            break;
        case 0x6:
            if (!(flags & SHIFT_USE_VX)) {
                x_value = y_value;
            }
            memory_set_register(mem, address, x_value >> 1);
            // Set to shifted out bis value, bit 1.
            memory_set_register(mem, 0xF, x_value & 0x01);
            break;
        case 0x7:
            memory_set_register(mem, address, y_value - x_value);
            memory_set_register(mem, 0xF, y_value >= x_value);
            break;
        case 0xe:
            if (!(flags & SHIFT_USE_VX)) {
                x_value = y_value;
            }
            memory_set_register(mem, address, x_value << 1);
            // Set to shifted out bis value, bit 8.
            memory_set_register(mem, 0xF, (x_value & 0x80) != 0);
            break;
        }
        break;
    case 0x9:
        if (memory_get_register(mem, getX(instruction)) !=
            memory_get_register(mem, getY(instruction))) {
            memory_skip_instruction(mem);
        }
        break;
    case 0xA:
        address = getNNN(instruction);
        memory_set_index_register(mem, address);
        break;
    case 0xB:
        address = getNNN(instruction);
        if (flags & JUMPING_USE_VX) {
            value = memory_get_register(mem, getX(instruction));
        } else {
            value = memory_get_register(mem, 0);
        }
        memory_set_instruction(mem, address + value);
        break;
    case 0xC:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(mem, address, (rand() % 0xFF) & value);
        break;
    case 0xD:
        if ((flags & DISPLAY_WAIT) && !ready) {
            memory_repeat_instruction(mem);
            break;
        }

        ready = false;
        uint8_t register_x = getX(instruction);
        uint8_t register_y = getY(instruction);
        uint8_t count = getN(instruction);
        uint8_t *sprite = memory_get_sprite(mem);

        int result =
            render_sprite(r, memory_get_register(mem, register_x) % WIDTH,
                          memory_get_register(mem, register_y) % HEIGHT, sprite,
                          count, (flags & DISPLAY_CLIPPING));
        if (result == 1) {
            memory_set_register(mem, 0xF, 1);
        } else {
            memory_set_register(mem, 0xF, 0);
        }
        break;
    case 0xE:
        uint8_t key = memory_get_register(mem, getX(instruction));
        if (code == 0)
            break;
        uint8_t key_code = scancode_to_key(code);
        if ((getNN(instruction)) == 0x9E && key == key_code) {
            memory_skip_instruction(mem);
        } else if ((getNN(instruction)) == 0xA1 && key != key_code &&
                   0xFF != code) {
            memory_skip_instruction(mem);
        }
        break;
    case 0xF:
        switch (getNN(instruction)) {
        case 0x07:
            memory_set_register(mem, getX(instruction),
                                memory_get_delay_timer(mem));
            break;
        case 0x15:
            memory_set_delay_timer(mem,
                                   memory_get_register(mem, getX(instruction)));
            break;
        case 0x18:
            memory_set_sound_timer(mem,
                                   memory_get_register(mem, getX(instruction)));
            break;
        case 0x1E:
            address = memory_get_index_register(mem);
            value = memory_get_register(mem, getX(instruction));
            memory_set_index_register(mem, address + value);
            if (address + value > 0xFFF) {
                memory_set_register(mem, 0xF, 1);
            }
            break;
        case 0x0A:
            uint8_t key_code = scancode_to_key(code);
            if (key_code == 0xFF) {
                memory_repeat_instruction(mem);
            } else {
                memory_set_register(mem, getX(instruction), key_code);
            }
            break;
        case 0x29:
            value = memory_get_register(mem, getX(instruction));
            memory_set_index_register(mem, 0x50 + value * 5);
            break;
        case 0x33:
            address = memory_get_index_register(mem);
            value = memory_get_register(mem, getX(instruction));
            uint8_t buffer = (value / 100);
            *(mem->memory + address) = buffer;
            buffer = (value % 100) / 10;
            *(mem->memory + address + 1) = buffer;
            buffer = (value % 10);
            *(mem->memory + address + 2) = buffer;
            break;
        case 0x55:
            memory_store_registers(mem, getX(instruction),
                                   flags & MEMORY_INDEX_INCREMENT);
            break;
        case 0x65:
            memory_load_registers(mem, getX(instruction),
                                  flags & MEMORY_INDEX_INCREMENT);
            break;
        }
    }
}

uint8_t scancode_to_key(SDL_Scancode code) {
    switch (code) {
    case SDL_SCANCODE_1:
        return 0x1;
    case SDL_SCANCODE_2:
        return 0x2;
    case SDL_SCANCODE_3:
        return 0x3;
    case SDL_SCANCODE_4:
        return 0xC;
    case SDL_SCANCODE_Q:
        return 0x4;
    case SDL_SCANCODE_W:
        return 0x5;
    case SDL_SCANCODE_E:
        return 0x6;
    case SDL_SCANCODE_R:
        return 0xD;
    case SDL_SCANCODE_A:
        return 0x7;
    case SDL_SCANCODE_S:
        return 0x8;
    case SDL_SCANCODE_D:
        return 0x9;
    case SDL_SCANCODE_F:
        return 0xE;
    case SDL_SCANCODE_Z:
        return 0xA;
    case SDL_SCANCODE_X:
        return 0x0;
    case SDL_SCANCODE_C:
        return 0xB;
    case SDL_SCANCODE_V:
        return 0xF;
    default:
        return 0xFF;
    }
}
