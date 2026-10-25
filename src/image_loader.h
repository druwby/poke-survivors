#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <SDL.h>

SDL_Surface* Image_LoadSurface(const char* filepath);
SDL_Texture* Image_LoadTexture(SDL_Renderer* renderer, const char* filepath);

#endif // IMAGE_LOADER_H
