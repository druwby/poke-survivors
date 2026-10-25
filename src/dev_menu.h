#ifndef DEV_MENU_H
#define DEV_MENU_H

#include "types.h"
#include <stdbool.h>

void DevMenu_Init(DevMenuState* menu);
void DevMenu_HandleEvent(GameState* game, const SDL_Event* event);
void DevMenu_Update(GameState* game);
void DevMenu_Draw(GameState* game);

#endif // DEV_MENU_H
