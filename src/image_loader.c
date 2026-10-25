#include "image_loader.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <stdio.h>
#include <string.h>

SDL_Surface* Image_LoadSurface(const char* filepath) {
    if (!filepath) return NULL;
    
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)filepath, strlen(filepath), false);
    if (!url) return NULL;
    
    CGImageSourceRef src = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    if (!src) return NULL;
    
    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, NULL);
    CFRelease(src);
    if (!img) return NULL;
    
    int w = (int)CGImageGetWidth(img);
    int h = (int)CGImageGetHeight(img);
    
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        CGImageRelease(img);
        return NULL;
    }
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        surface->pixels, w, h, 8, surface->pitch, colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(colorSpace);
    if (!ctx) {
        SDL_FreeSurface(surface);
        CGImageRelease(img);
        return NULL;
    }
    
    // Draw image directly into surface pixels (right-side up)
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
    CGContextRelease(ctx);
    CGImageRelease(img);
    
    return surface;
}

SDL_Texture* Image_LoadTexture(SDL_Renderer* renderer, const char* filepath) {
    SDL_Surface* surface = Image_LoadSurface(filepath);
    if (!surface) {
        printf("[ImageLoader] Warning: Failed to load image: %s\n", filepath);
        return NULL;
    }
    
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}
