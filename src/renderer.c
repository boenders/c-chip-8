#include "renderer.h"
#include "SDL3/SDL_render.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool lineup_pixels(renderer *r);

renderer *renderer_init(SDL_Renderer *sdl_renderer) {
    renderer *r = malloc(sizeof(renderer));
    r->sdl_renderer = sdl_renderer;
    if (!lineup_pixels(r)) {
        return NULL;
    }
    return r;
}
void renderer_free(renderer *r) { free(r); }

int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite,
                  uint8_t height) {
    uint64_t *row = r->bits + y;
    int result = 0;
    // Each draw is 4px wide
    for (int i = 0; i < height; i++) {
        // Put sprite in the beginning
        uint64_t coloring = ((*sprite));
        if (x > 56) {
            coloring = coloring >> (x - 56);
        } else {
            coloring = coloring << (56 - x);
        }
        // Check for overriden bits
        if ((*row ^ coloring) != (*row | coloring)) {
            result = 1;
        }
        *row = *row ^ coloring;
        sprite += 1;
        row += 1;
        if (i + y + 1 == HEIGHT)
            break;
    }
    if (!render_image(r, r->bits)) {
        result = 2;
    }
    return result;
}

bool render_clear(renderer *r) {
    for (int i = 0; i < HEIGHT; i++) {
        r->bits[i] = 0;
    }
    return render_image(r, r->bits);
}

bool render_image(renderer *r, uint64_t *bits) {
    uint64_t *target = bits;
    uint64_t comparator = 1;
    comparator = comparator << 63;
    SDL_FRect rects_to_render[WIDTH * HEIGHT];
    int count = 0;
    for (size_t i = 0; i < HEIGHT * WIDTH; i++) {
        if ((*target & comparator) != 0) {
            // render white
            rects_to_render[count++] = r->pixels[i];
        }
        if ((comparator >> 1) == 0) {
            comparator = comparator << 63;
            target += 1;
        } else {
            comparator = comparator >> 1;
        }
    }
    SDL_SetRenderDrawColor(r->sdl_renderer, 0, 0, 0, 0);
    SDL_RenderClear(r->sdl_renderer);
    SDL_SetRenderDrawColor(r->sdl_renderer, 255, 255, 255, 255);
    SDL_RenderFillRects(r->sdl_renderer, rects_to_render, count);
    SDL_RenderPresent(r->sdl_renderer);
    return true;
}

bool lineup_pixels(renderer *r) {
    int width = 0, height = 0;
    if (!SDL_GetRenderOutputSize(r->sdl_renderer, &width, &height)) {
        return false;
    }
    float rect_width = (float)width / WIDTH;
    float rect_height = (float)height / HEIGHT;

    for (int i = 0; i < HEIGHT; i++) {
        for (int k = 0; k < WIDTH; k++) {
            r->pixels[i * WIDTH + k].x = k * rect_width;
            r->pixels[i * WIDTH + k].y = i * rect_height;
            r->pixels[i * WIDTH + k].w = rect_width;
            r->pixels[i * WIDTH + k].h = rect_height;
        }
    }
    return true;
}
