#ifndef RENDERER
#define RENDERER
#include "SDL3/SDL_render.h"

#define WIDTH 64
#define HEIGHT 32

typedef struct {
    SDL_Renderer *sdl_renderer;
    SDL_FRect pixels[WIDTH * HEIGHT];
    uint64_t bits[WIDTH*HEIGHT / 64];
} renderer;
/**
 * Initializes a new renderer with a screen size of 64 by 32 pixels.
 *
 * During initialization, the renderer adapts itself to the opened size
 * once and aligns the pixels correctly according to the window size. If the 
 * size of the window changes at some point this needs to be redone by calling
 * renderer_lineup_pixels.
 */
renderer *renderer_init(SDL_Renderer *sdl_renderer);
void renderer_free(renderer *r);
int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite, uint8_t count);
bool render_image(renderer *r, uint64_t *bits);
bool render_clear(renderer *r);
bool renderer_lineup_pixels(renderer *r);
#endif // !RENDERER
