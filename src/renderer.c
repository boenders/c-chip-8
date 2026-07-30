#include "renderer.h"
#include "SDL3/SDL_render.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool renderer_lineup_pixels(renderer *r);

renderer *renderer_init(SDL_Renderer *sdl_renderer) {
    renderer *r = calloc(1, sizeof *r);
    memset(r->pixel_values, 0, sizeof(r->pixel_values));
    memset(r->pixels, 0, sizeof(r->pixels));
    r->sdl_renderer = sdl_renderer;
    if (!renderer_lineup_pixels(r)) {
        return NULL;
    }
    return r;
}
void renderer_free(renderer *r) { free(r); }

bool render_image(renderer *r) {
    // To make rendering easy, the screen is first set to black everywhere.
    //
    // Afterwards the rectangles that have been determined to be enabled
    // will be redrawn as white rectangles.
    SDL_SetRenderDrawColor(r->sdl_renderer, BLACK_PIXEL, BLACK_PIXEL,
                           BLACK_PIXEL, 255);
    SDL_RenderClear(r->sdl_renderer);
    // Render each rectangle onto the screen. As each one can fade, leading to
    // many unpredictable colors the rectangles are rendered directly. There are
    // very few rectangles anyways so this is not a performance concern.
    for (size_t i = 0; i < HEIGHT * WIDTH; i++) {
        if (r->pixel_values[i] > 0) {
            // If the pixel is not fully bright it is being dimmed. The dimming
            // should continue to gain a smooth looking rendering.
            if (r->pixel_values[i] < ENABLED_PIXEL) {
                if (ENABLED_PIXEL - r->pixel_values[i] > r->pixel_values[i]) {
                    r->pixel_values[i] = 0;
                } else {
                    r->pixel_values[i] -= ENABLED_PIXEL - r->pixel_values[i];
                }
            }

            uint8_t offset = (ENABLED_PIXEL - r->pixel_values[i]);
            uint8_t color = offset > (WHITE_PIXEL - BLACK_PIXEL)
                                ? BLACK_PIXEL
                                : WHITE_PIXEL - offset;

            SDL_SetRenderDrawColor(r->sdl_renderer, color, color, color, 255);
            SDL_RenderFillRect(r->sdl_renderer, r->pixels + i);
        }
    }
    SDL_RenderPresent(r->sdl_renderer);
    return true;
}

int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite,
                  uint8_t height, bool clipping) {
    int result = 0;
    // Each draw is 8px wide, so only the height needs to be considered as a
    // parameter.
    for (int i = 0; i < height; i++) {
        uint32_t row = y + i;
        // Depending on if clipping is enabled the rendering stops
        // or continues on the top part of the display.
        if (row >= HEIGHT && !clipping) {
            break;
        } else {
            row %= HEIGHT;
        }
        // The 8px wide sprite is represented by a single u8 where each bit is a
        // position on the screen. These bits are masked of and checked one
        // after the other to set the corresponding pixel on the visualization
        // array.
        for (int k = 0; k < 8; k++) {
            uint32_t position;
            // Ambigous case, depending on the clipping flag a color will be
            // wrapped to the other side or be clipped because it is off
            // the screen.
            if (x + k >= WIDTH && clipping) {
                position = row * WIDTH + ((x + k) % WIDTH);
            } else if (x + k >= WIDTH) {
                break;
            } else {
                position = row * WIDTH + x + k;
            }
            // If any bits will be overridden during the sprite rendering
            // process the VF register will need to be set to one, to achieve
            // this, the result 1 is returned which is interpretable by the
            // calling function.
            if ((*(sprite + i) & (128 >> k)) != 0) {
                // If the pixel is already enabled, it will be set to the
                // dimming state. This means any value. This way the renderer
                // will later slowly hide the pixel with time smoothly.
                //
                // The pixel value is a uint8 where 255 is ENABLED and 240 is
                // DIMMING as the start of the dimming process.
                if (r->pixel_values[position] == ENABLED_PIXEL) {
                    r->pixel_values[position] = DIMMING_PIXEL;
                } else {
                    // If the pixel was not enabled previously it is set to
                    // enabled.
                    r->pixel_values[position] = ENABLED_PIXEL;
                }
                // A pixel has changed so result has to be set to one.
                result = 1;
            }
        }
    }
    if (!render_image(r)) {
        result = 2;
    }
    return result;
}

bool render_clear(renderer *r) {
    memset(r->pixel_values, 0, sizeof(r->pixel_values));
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
