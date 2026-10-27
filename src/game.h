#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "spatial_grid.h"

void Game_Init(GameState* game, SDL_Window* window, SDL_Renderer* renderer);
void Game_HandleInput(GameState* game, const Uint8* keystate, float dt);
void Game_HandleEvent(GameState* game, const SDL_Event* event);
void Game_Update(GameState* game, float dt);
void Game_Draw(GameState* game);
void Game_Cleanup(GameState* game);
void Game_ActivateEnemy(GameState* game, int e_idx);
void Game_DeactivateEnemy(GameState* game, int e_idx);
void Game_SelectStarter(GameState* game, StarterID starter);
void Game_GenerateLevelUpCards(GameState* game);
void Game_ApplyLevelUpChoice(GameState* game, int card_index);
void Game_ThrowPokeball(GameState* game);
bool Party_HasType(const GameState* game, PokeType type);
void Party_Update(GameState* game, float dt);
void Party_Draw(GameState* game);

#endif // GAME_H
