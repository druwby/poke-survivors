#ifndef FONT_H
#define FONT_H

#include <SDL.h>
#include <string.h>

// Clean embedded 8x8 font bitmap for ASCII 32 to 126 (95 glyphs)
// Each glyph is 8 rows of 8 bits (1 byte per row, MSB on left)
extern const unsigned char FONT_8X8[95][8];

SDL_Texture* Font_CreateAtlas(SDL_Renderer* renderer);
void Font_DrawText(SDL_Renderer* renderer, SDL_Texture* atlas, const char* text, int x, int y, int scale, SDL_Color color);
void Font_DrawTextShadow(SDL_Renderer* renderer, SDL_Texture* atlas, const char* text, int x, int y, int scale, SDL_Color color);

#endif // FONT_H
