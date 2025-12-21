#ifndef RENDERER
#define RENDERER
#include "SDL3/SDL_render.h"

#define WIDTH 64
#define HEIGHT 32

typedef struct {
    SDL_Renderer *sdl_renderer;
    // Rectangles with positions and sizes set to work on the screen. 
    //
    // Prepared to be used during the rendering process.
    SDL_FRect pixels[WIDTH * HEIGHT];
    // Constant 64 as each bit in the 64-bit integer will represent one pixel
    // on the screen.
    //
    // Using 64-bit ints is very convenient as each value in the array will
    // represent exactly one full row of the display making it very easy to
    // work with.
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
/**
 * Frees an allocated renderer instance.
 */
void renderer_free(renderer *r);
/**
 * Renders a single sprite to the screen.
 *
 * The position of the rendered sprite is provided by the x and y parameters.
 * The sprite parameter has to point to the memory location with the beginning
 * of the sprite.
 * The count parameter determines how many bytes of the sprite will be drawn,
 * drawing one byte per row.
 *
 * The clipping flag can be set to enable ambigous behavior for some ROMs
 */
int render_sprite(renderer *r, uint8_t x, uint8_t y, uint8_t *sprite, uint8_t count, bool clipping);
/**
 * Sets all pixels to zero. 
 *
 * The changes will immediately be drawn to the sccreen.
 */
bool render_clear(renderer *r);
/**
 * Adapts the pixels of the renderer to the display size. 
 *
 * This is only necessary when the window size has changed, otherwise a 
 * realignment is not necessary.
 */
bool renderer_lineup_pixels(renderer *r);
#endif // !RENDERER
