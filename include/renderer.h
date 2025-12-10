#ifndef RENDERER
#define RENDERER

#define WIDTH 64
#define HEIGHT 32

#include "SDL3/SDL_render.h"
typedef struct {
    SDL_Renderer *sdl_renderer;
    SDL_FRect pixels[WIDTH * HEIGHT];
    uint64_t bits[WIDTH*HEIGHT / 64];
} renderer;
renderer *renderer_init(SDL_Renderer *sdl_renderer);
void renderer_free(renderer *r);
int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite, uint8_t count);
bool render_image(renderer *r, uint64_t *bits);
bool render_clear(renderer *r);
#endif // !RENDERER
