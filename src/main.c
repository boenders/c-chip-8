#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "memory_subsystem.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void decode(uint16_t instruction, memory_subsystem *mem, renderer *r,
            SDL_Scancode code);
uint8_t scancode_to_key(SDL_Scancode code);

int main(int argc, char **argv) {
    printf("Got %i arguments", argc);
    if (argc < 2) {
        printf("Too few arguments, specify a game file");
        return 1;
    }
    char *filepath = *(argv + 1);

    int rc = 0;
    FILE *fptr = fopen(filepath, "rb");
    if (!fptr) {
        rc = 1;
        goto exit;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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
                // if ((*(SDL_KeyboardEvent *)&e).down) {
                code = (*(SDL_KeyboardEvent *)&e).scancode;
                // } else if ((*(SDL_KeyboardEvent *)&e).down) {
                //     ke = NULL;
                // }
            }
            if (e.type == SDL_EVENT_KEY_UP) {
                code = 0;
            }
        }
        // Fetch
        uint16_t instruction = get_instruction(mem);
        decode(instruction, mem, r, code);

        // Decode
        // Execute
        // Every 16ms
        if (16000000 < SDL_GetTicksNS() - timestamp) {
            timestamp = SDL_GetTicksNS();

            if (mem->delay_timer > 0) {
                mem->delay_timer--;
            }
            if (mem->sound_timer > 0) {
                mem->sound_timer--;
            }
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

#define getX(v) (v >> 8) & 0x000F
#define getY(v) (v >> 4) & 0x000F
#define getN(v) v & 0x000F
#define getNN(v) v & 0x00FF
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
            jump_back(mem);
        }
        break;
    case 0x1:
        address = getNNN(instruction);
        // fprintf(stdout, "Got address to run: %x\n", address);
        // fflush(stdout);
        set_instruction(mem, address);
        break;
    case 0x2:
        address = getNNN(instruction);
        jump_to(mem, address);
        break;
    case 0x3:
        value = getNN(instruction);
        if (get_register(mem, getX(instruction)) == value) {
            skip_instruction(mem);
        }
        break;
    case 0x4:
        value = getNN(instruction);
        if (get_register(mem, getX(instruction)) != value) {
            skip_instruction(mem);
        }
        break;
    case 0x5:
        if (get_register(mem, getX(instruction)) ==
            get_register(mem, getY(instruction))) {
            skip_instruction(mem);
        }
        break;
    case 0x6:
        address = getX(instruction);
        value = getNN(instruction);
        set_register(mem, address, value);
        break;
    case 0x7:
        address = getX(instruction);
        value = getNN(instruction);
        set_register(mem, address, get_register(mem, address) + value);
        break;
    case 0x8:
        address = getX(instruction);
        uint8_t x_value = get_register(mem, getX(instruction));
        uint8_t y_value = get_register(mem, getY(instruction));
        switch (getN(instruction)) {
        case 0x0:
            set_register(mem, address, y_value);
            break;
        case 0x1:
            set_register(mem, address, x_value | y_value);
            break;
        case 0x2:
            set_register(mem, address, x_value & y_value);
            break;
        case 0x3:
            set_register(mem, address, x_value ^ y_value);
            break;
        case 0x4:
            set_register(mem, address, x_value + y_value);
            set_register(mem, 0xF, x_value + y_value > UINT8_MAX);
            break;
        case 0x5:
            set_register(mem, address, x_value - y_value);
            set_register(mem, 0xF, x_value > y_value);
            break;
        case 0x6:
            set_register(mem, 0xF, (x_value & 0x01) != 0);
            set_register(mem, address, x_value >> 1);
            break;
        case 0x7:
            set_register(mem, 0xF, y_value > x_value);
            set_register(mem, address, y_value - x_value);
            break;
        case 0xe:
            set_register(mem, 0xF, (x_value & 0x80) != 0);
            set_register(mem, address, x_value << 1);
            break;
        }
        break;
    case 0x9:
        if (get_register(mem, getX(instruction)) !=
            get_register(mem, getY(instruction))) {
            skip_instruction(mem);
        }
        break;
    case 0xA:
        address = getNNN(instruction);
        set_index_register(mem, address);
        break;
    case 0xB:
        address = getNNN(instruction);
        value = get_register(mem, getX(instruction));
        set_instruction(mem, address + value);
        break;
    case 0xC:
        address = getX(instruction);
        value = getNN(instruction);
        set_register(mem, address, (rand() % 0xFF) & value);
        break;
    case 0xD:
        uint8_t register_x = getX(instruction);
        uint8_t register_y = getY(instruction);
        uint8_t count = getN(instruction);
        uint8_t *sprite = get_sprite(mem);

        int result = render_sprite(r, get_register(mem, register_x) % WIDTH,
                                   get_register(mem, register_y) % HEIGHT,
                                   sprite, count);
        if (result == 1) {
            set_register(mem, 0xF, 1);
        }
        break;
    case 0xE:
        uint8_t key = get_register(mem, getX(instruction));
        if (code == 0)
            break;
        uint8_t key_code = scancode_to_key(code);
        printf("Got Code translated: %i\n", key_code);
        if ((getNN(instruction)) == 0x9E && key == key_code) {
            skip_instruction(mem);
        } else if ((getNN(instruction)) == 0xA1 && key != key_code &&
                   0xFF != code) {
            skip_instruction(mem);
        }
        break;
    case 0xF:
        switch (getNN(instruction)) {
        case 0x07:
            set_register(mem, getX(instruction), mem->delay_timer);
            break;
        case 0x15:
            mem->delay_timer = get_register(mem, getX(instruction));
            break;
        case 0x18:
            mem->sound_timer = get_register(mem, getX(instruction));
            break;
        case 0x1E:
            mem->index_register += get_register(mem, getX(instruction));
            if (mem->index_register > 0xFFF) {
                set_register(mem, 0xF, 1);
            }
            break;
        case 0x0A:
            if (code == 0) {
                repeat_instruction(mem);
            } else {
                uint8_t key_code = scancode_to_key(code);
                printf("Got Code translated: %i\n", key_code);
                if (key_code == 0xFF) {
                    repeat_instruction(mem);
                } else {
                    set_register(mem, getX(instruction), key_code);
                }
            }
            break;
        case 0x29:
            value = get_register(mem, getX(instruction));
            set_index_register(mem, 0x50 + value * 5);
            break;
        case 0x33:
            value = get_register(mem, getX(instruction));
            printf("Setting registers using FX33 to %i\n", value);
            uint8_t buffer = (value / 100);
            printf("Got value %i\n", buffer);
            *(mem->memory + mem->index_register) = buffer;
            buffer = (value % 100) / 10;
            printf("Got value %i\n", buffer);
            *(mem->memory + mem->index_register + 1) = buffer;
            buffer = (value % 10);
            printf("Got value %i\n", buffer);
            *(mem->memory + mem->index_register + 2) = buffer;
            printf("Registers were set to %x %x %x\n",
                   *(uint8_t *)(mem->memory + mem->index_register),
                   *(uint8_t *)(mem->memory + mem->index_register + 1),
                   *(uint8_t *)(mem->memory + mem->index_register + 2));
            break;
        case 0x55:
            store_registers(mem, getX(instruction));
            break;
        case 0x65:
            load_registers(mem, getX(instruction));
            break;
        }
    }
}

uint8_t scancode_to_key(SDL_Scancode code) {
    printf("Got Code No: %i\n", code);
    switch (code) {
    case SDL_SCANCODE_1:
        return 0x0;
    case SDL_SCANCODE_2:
        return 0x1;
    case SDL_SCANCODE_3:
        return 0x2;
    case SDL_SCANCODE_4:
        return 0x3;
    case SDL_SCANCODE_Q:
        return 0x4;
    case SDL_SCANCODE_W:
        return 0x5;
    case SDL_SCANCODE_E:
        return 0x6;
    case SDL_SCANCODE_R:
        return 0x7;
    case SDL_SCANCODE_A:
        return 0x8;
    case SDL_SCANCODE_S:
        return 0x9;
    case SDL_SCANCODE_D:
        return 0xA;
    case SDL_SCANCODE_F:
        return 0xB;
    case SDL_SCANCODE_Y:
        return 0xC;
    case SDL_SCANCODE_X:
        return 0xD;
    case SDL_SCANCODE_C:
        return 0xE;
    case SDL_SCANCODE_V:
        return 0xF;
    default:
        return 0xFF;
    }
}
