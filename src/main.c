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
#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

#define FRAMETIME 16000000 // 16 ms

typedef struct {
    // Initial setup state;
    uint8_t flags;
    bool display_ready;
    SDL_Window *win;
    SDL_Renderer *ren;
    renderer *r;
    memory_subsystem *mem;

    // Running application state;
    int running;
    int old_width;
    int old_height;
    uint64_t timestamp;
    SDL_Scancode code;
} AppState;

void decode(uint16_t instruction, AppState *state);
uint8_t scancode_to_key(SDL_Scancode code);
void print_help();

void tick(void *arg) {
    AppState *state = (AppState *)arg;
    SDL_Event e;
    // Catch all SDL events outside of the normal c-chip logic to keep it
    // separate.
    //
    // Keypresses are saved to a variable that is passed to the decode
    // function. Only the last keypress is held in this variable, if
    // multiple keys are pressed only the last one is forwarded.
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT ||
            e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            state->running = 0;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
        }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            state->code = (*(SDL_KeyboardEvent *)&e).scancode;
        }
        if (e.type == SDL_EVENT_KEY_UP) {
            state->code = 0;
        }
    }

    int current_width = 0;
    int current_height = 0;
    /**
     * Track the screen size outside of the normal chip-8 logic as this
     * would not be a part of it normally. If any change in size is
     * detected realign the renderer pixels to keep using the full screen.
     */
    SDL_GetRenderOutputSize(state->ren, &current_width, &current_height);
    if (state->old_width != current_width ||
        state->old_height != current_height) {
        state->old_width = current_width;
        state->old_height = current_height;
        renderer_lineup_pixels(state->r);
    }

    // When display waiting is enabled (default=true) only one sprite
    // can be drawn per frame. The application targets a refresh rate
    // of 60 (the same as the chip-8), so putting it in the timer loop
    // is a sensible choice.
    //
    // True equals the display being able to take another sprite.
    state->display_ready = true;

    state->timestamp = SDL_GetTicksNS();
    if (memory_get_delay_timer(state->mem) > 0) {
        memory_set_delay_timer(state->mem,
                               memory_get_delay_timer(state->mem) - 1);
    }
    if (memory_get_sound_timer(state->mem) > 0) {
        memory_set_sound_timer(state->mem,
                               memory_get_sound_timer(state->mem) - 1);
    }

    // Main logic loop of the chip-8, getting the next instruction and then
    // decoding + executing it in one step.
    //
    // This is run 11 times to reach around 660 operations per second at 60 fps,
    // close to the real value of the chip-8
    for (int i = 0; i < 11; i++) {
        uint16_t instruction = memory_get_instruction(state->mem);
        decode(instruction, state);
    }
}

int main(int argc, char **argv) {
    // Flags to enable/disable quirky behavior depending on application
    // requirements.
    uint8_t flags =
        VF_RESET | MEMORY_INDEX_INCREMENT | DISPLAY_WAIT | DISPLAY_CLIPPING;
    // Flag used to slow sprite rendering to 60 sprites a second.
    bool display_ready = true;

    if (argc < 2) {
        print_help();
        return 1;
    }
    // Very simple solution to check for flags that appear in the arguments.
    //
    // Due to the simplicity, specifying the same flag multiple times will
    // negate itself, e.g. twice is the same as not at all.
    //
    // A more complex, easier to extend solution would be possible but is not
    // necessary for the small project.
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
        printf("Could not open file: %s\n\n", *(argv + argc - 1));
        print_help();
        rc = 1;
        goto exit;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        rc = 1;
        goto cleanup_file;
    }

    SDL_Window *win =
        SDL_CreateWindow("Chip-8", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        rc = 1;
        goto cleanup_file;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        rc = 1;
        goto cleanup_window;
    }
    // SDL_SetRenderVSync(ren, 1);

    renderer *r = renderer_init(ren);
    memory_subsystem *mem = memory_init();
    if (!r || !mem) {
        fprintf(stderr, "Renderer or memory subsystem init error: %s\n",
                SDL_GetError());
        rc = 1;
        goto cleanup_sdl_renderer;
    }

    if (!fread(mem->memory + PROGRAM_START, 1, MEMORY - PROGRAM_START, fptr)) {
        rc = 1;
        goto cleanup;
    } else {
        fclose(fptr);
        fptr = NULL;
    }

    int running = 1;
    int old_width = 0;
    int old_height = 0;
    uint64_t timestamp = SDL_GetTicksNS();
    SDL_Scancode code;
    AppState *state = malloc(sizeof(*state));
    if (state == NULL) {
        goto cleanup;
    }

    state->flags = flags;
    state->display_ready = display_ready;
    state->win = win;
    state->ren = ren;
    state->r = r;
    state->mem = mem;

    state->running = running;
    state->old_width = old_width;
    state->old_height = old_height;
    state->timestamp = timestamp;
    state->code = code;

#ifdef PLATFORM_WEB
    emscripten_set_main_loop_arg(tick, state, 0, 1);
#else
    while (running) {
        uint64_t start = SDL_GetTicksNS();

        tick(state);
        if (state->running == 0) {
            break;
        }
        uint64_t runtime = SDL_GetTicksNS() - start;
        if (runtime < FRAMETIME) {
            SDL_DelayNS(FRAMETIME - runtime);
        }
    }
#endif

    free(state);
cleanup:
    memory_free(mem);
    renderer_free(r);
cleanup_sdl_renderer:
    SDL_DestroyRenderer(ren);
cleanup_window:
    SDL_DestroyWindow(win);
    SDL_Quit();
cleanup_file:
    if (fptr != NULL) {
        fclose(fptr);
    }
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

void decode(uint16_t instruction, AppState *state) {
    uint16_t address;
    uint8_t value;
    uint8_t value_two;
    uint8_t key_code;

    switch ((instruction >> 12) & 0x000F) {
    case 0x0:
        if (instruction == 0x00E0) {
            render_clear(state->r);
        } else if (instruction == 0x00EE) {
            memory_instruction_jump_back(state->mem);
        }
        break;
    case 0x1:
        address = getNNN(instruction);
        memory_set_instruction(state->mem, address);
        break;
    case 0x2:
        address = getNNN(instruction);
        memory_instruction_jump_to(state->mem, address);
        break;
    case 0x3:
        value = getNN(instruction);
        if (memory_get_register(state->mem, getX(instruction)) == value) {
            memory_skip_instruction(state->mem);
        }
        break;
    case 0x4:
        value = getNN(instruction);
        if (memory_get_register(state->mem, getX(instruction)) != value) {
            memory_skip_instruction(state->mem);
        }
        break;
    case 0x5:
        if (memory_get_register(state->mem, getX(instruction)) ==
            memory_get_register(state->mem, getY(instruction))) {
            memory_skip_instruction(state->mem);
        }
        break;
    case 0x6:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(state->mem, address, value);
        break;
    case 0x7:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(state->mem, address,
                            memory_get_register(state->mem, address) + value);
        break;
    case 0x8:
        address = getX(instruction);
        uint8_t x_value = memory_get_register(state->mem, getX(instruction));
        uint8_t y_value = memory_get_register(state->mem, getY(instruction));
        switch (getN(instruction)) {
        case 0x0:
            memory_set_register(state->mem, address, y_value);
            break;
        case 0x1:
            memory_set_register(state->mem, address, x_value | y_value);
            if (state->flags & VF_RESET) {
                memory_set_register(state->mem, 0xF, 0);
            }
            break;
        case 0x2:
            memory_set_register(state->mem, address, x_value & y_value);
            if (state->flags & VF_RESET) {
                memory_set_register(state->mem, 0xF, 0);
            }
            break;
        case 0x3:
            memory_set_register(state->mem, address, x_value ^ y_value);
            if (state->flags & VF_RESET) {
                memory_set_register(state->mem, 0xF, 0);
            }
            break;
        case 0x4:
            memory_set_register(state->mem, address, x_value + y_value);
            memory_set_register(state->mem, 0xF, x_value + y_value > UINT8_MAX);
            break;
        case 0x5:
            memory_set_register(state->mem, address, x_value - y_value);
            memory_set_register(state->mem, 0xF, x_value >= y_value);
            break;
        case 0x6:
            if (!(state->flags & SHIFT_USE_VX)) {
                x_value = y_value;
            }
            memory_set_register(state->mem, address, x_value >> 1);
            // Set to shifted out bis value, bit 1.
            memory_set_register(state->mem, 0xF, x_value & 0x01);
            break;
        case 0x7:
            memory_set_register(state->mem, address, y_value - x_value);
            memory_set_register(state->mem, 0xF, y_value >= x_value);
            break;
        case 0xe:
            if (!(state->flags & SHIFT_USE_VX)) {
                x_value = y_value;
            }
            memory_set_register(state->mem, address, x_value << 1);
            // Set to shifted out bis value, bit 8.
            memory_set_register(state->mem, 0xF, (x_value & 0x80) != 0);
            break;
        }
        break;
    case 0x9:
        if (memory_get_register(state->mem, getX(instruction)) !=
            memory_get_register(state->mem, getY(instruction))) {
            memory_skip_instruction(state->mem);
        }
        break;
    case 0xA:
        address = getNNN(instruction);
        memory_set_index_register(state->mem, address);
        break;
    case 0xB:
        address = getNNN(instruction);
        if (state->flags & JUMPING_USE_VX) {
            value = memory_get_register(state->mem, getX(instruction));
        } else {
            value = memory_get_register(state->mem, 0);
        }
        memory_set_instruction(state->mem, address + value);
        break;
    case 0xC:
        address = getX(instruction);
        value = getNN(instruction);
        memory_set_register(state->mem, address, (rand() % 0xFF) & value);
        break;
    case 0xD:
        // The display_ready counter for sprite drawing is only considered
        // when the display wait flag is set.
        if ((state->flags & DISPLAY_WAIT) && !state->display_ready) {
            memory_repeat_instruction(state->mem);
            break;
        }

        state->display_ready = false;
        uint8_t register_x = getX(instruction);
        uint8_t register_y = getY(instruction);
        uint8_t count = getN(instruction);
        uint8_t *sprite = memory_get_sprite(state->mem);

        int result = render_sprite(
            state->r, memory_get_register(state->mem, register_x) % WIDTH,
            memory_get_register(state->mem, register_y) % HEIGHT, sprite, count,
            (state->flags & DISPLAY_CLIPPING));
        if (result == 1) {
            memory_set_register(state->mem, 0xF, 1);
        } else {
            memory_set_register(state->mem, 0xF, 0);
        }
        break;
    case 0xE:
        key_code = scancode_to_key(state->code);
        uint8_t key = memory_get_register(state->mem, getX(instruction));
        if ((getNN(instruction)) == 0x9E && key == key_code) {
            memory_skip_instruction(state->mem);
        } else if ((getNN(instruction)) == 0xA1 && key != key_code) {
            memory_skip_instruction(state->mem);
        }
        break;
    case 0xF:
        switch (getNN(instruction)) {
        case 0x07:
            memory_set_register(state->mem, getX(instruction),
                                memory_get_delay_timer(state->mem));
            break;
        case 0x15:
            memory_set_delay_timer(
                state->mem, memory_get_register(state->mem, getX(instruction)));
            break;
        case 0x18:
            memory_set_sound_timer(
                state->mem, memory_get_register(state->mem, getX(instruction)));
            break;
        case 0x1E:
            address = memory_get_index_register(state->mem);
            value = memory_get_register(state->mem, getX(instruction));
            memory_set_index_register(state->mem, address + value);
            if (address + value > 0xFFF) {
                memory_set_register(state->mem, 0xF, 1);
            }
            break;
        case 0x0A:
            key_code = scancode_to_key(state->code);
            if (key_code == 0xFF) {
                memory_repeat_instruction(state->mem);
            } else {
                memory_set_register(state->mem, getX(instruction), key_code);
            }
            break;
        case 0x29:
            value = memory_get_register(state->mem, getX(instruction));
            memory_set_index_register(state->mem, 0x50 + value * 5);
            break;
        case 0x33:
            // Split the number in three parts and set registers starting from
            // the index register.
            // 257
            // index_register = 2
            // index_register+1 = 5
            // index_register+2 = 7
            address = memory_get_index_register(state->mem);
            value = memory_get_register(state->mem, getX(instruction));
            uint8_t buffer = (value / 100);
            *(state->mem->memory + address) = buffer;
            buffer = (value % 100) / 10;
            *(state->mem->memory + address + 1) = buffer;
            buffer = (value % 10);
            *(state->mem->memory + address + 2) = buffer;
            break;
        case 0x55:
            memory_store_registers(state->mem, getX(instruction),
                                   state->flags & MEMORY_INDEX_INCREMENT);
            break;
        case 0x65:
            memory_load_registers(state->mem, getX(instruction),
                                  state->flags & MEMORY_INDEX_INCREMENT);
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

void print_help() {
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
}
