#include "renderer.h"
#include "SDL3/SDL_render.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool renderer_lineup_pixels(renderer *r);

renderer *renderer_init(SDL_Renderer *sdl_renderer) {
    renderer *r = calloc(1, sizeof *r);
    memset(r->bits, 0, sizeof(r->bits));
    memset(r->pixels, 0, sizeof(r->pixels));
    r->sdl_renderer = sdl_renderer;
    if (!renderer_lineup_pixels(r)) {
        return NULL;
    }
    return r;
}
void renderer_free(renderer *r) { free(r); }

bool render_image(renderer *r) {
    uint64_t *target = r->bits;
    uint64_t comparator = 1;
    comparator = comparator << 63;
    SDL_FRect rects_to_render[WIDTH * HEIGHT];
    int count = 0;
    // Determine the pixels to to redraw by stepping through the bits of the
    // rows. The bits are stepped through and each bit represents a pixel.
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
    // To make rendering easy, the screen is first set to black everywhere.
    //
    // Afterwards the rectangles that have been determined to be enabled
    // will be redrawn as white rectangles.
    SDL_SetRenderDrawColor(r->sdl_renderer, 22, 22, 22, 255);
    SDL_RenderClear(r->sdl_renderer);
    SDL_SetRenderDrawColor(r->sdl_renderer, 200, 200, 200, 255);
    SDL_RenderFillRects(r->sdl_renderer, rects_to_render, count);
    SDL_RenderPresent(r->sdl_renderer);
    return true;
}

int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite,
                  uint8_t height, bool clipping) {
    uint64_t *row = r->bits + y;
    int result = 0;
    // Each draw is 8px wide, so only the height needs to be considered.
    for (int i = 0; i < height; i++) {
        // Like the rows the coloring variable uses a uint64 as this
        // conveniently represents one whole row of the display.
        //
        // The sprite is initially positioned in the last eight bits of the
        // coloring int. Depending on x the coloring will be shifted to the
        // correct position. the offset constant is 56 (64 - 8), as this will
        // shift the sprite all the way to the first bit when x = 0.
        uint64_t coloring = ((*sprite));
        if (x > 56) {
            // Ambigous case, depending on the flag a color will be wrapped
            // to the other side or be clipped because it is off the screen.
            if (clipping) {
                coloring = coloring >> (x - 56);
            } else {
                coloring = coloring >> (x - 56) | coloring << (56 - x);
            }
        } else {
            coloring = coloring << (56 - x);
        }
        // Check for overridden bits here.
        //
        // If any bits will be overridden during the sprite rendering process
        // the VF register will need to be set to one, to achieve this, the
        // result 1 is returned which is interpretable by the calling function.
        if ((*row ^ coloring) != (*row | coloring)) {
            result = 1;
        }
        *row = *row ^ coloring;
        sprite += 1;
        row += 1;
        if (i + y + 1 == HEIGHT) {
            // Depending on if clipping is enabled the rendering can just stop
            // or continue on the top part of the display.
            if (clipping) {
                break;
            } else {
                row = r->bits;
            }
        }
    }
    if (!render_image(r)) {
        result = 2;
    }
    return result;
}

bool render_clear(renderer *r) {
    for (int i = 0; i < HEIGHT; i++) {
        r->bits[i] = 0;
    }
    return render_image(r);
}

bool renderer_lineup_pixels(renderer *r) {
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
