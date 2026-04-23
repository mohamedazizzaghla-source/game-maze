#ifndef MENU_PERSONNAGE_H
#define MENU_PERSONNAGE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "joueur.h"

int menu_personnage(SDL_Renderer *renderer, TTF_Font *font, const char *titre_joueur,
                    JoueurConfig *cfg);

#endif
