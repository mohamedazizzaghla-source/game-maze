#include "menu_personnage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char *nom;
    Uint8 r, g, b;
} Teinte;

static const Teinte *obtenir_teintes(int *nb_teintes)
{
    static Teinte *teintes = NULL;
    if (!teintes) {
        teintes = (Teinte *)malloc(6 * sizeof(Teinte));
        if (!teintes) {
            *nb_teintes = 0;
            return NULL;
        }
        teintes[0] = (Teinte){"Rouge", 200, 60, 60};
        teintes[1] = (Teinte){"Vert", 60, 180, 90};
        teintes[2] = (Teinte){"Bleu", 70, 120, 220};
        teintes[3] = (Teinte){"Jaune", 220, 200, 60};
        teintes[4] = (Teinte){"Violet", 150, 80, 200};
        teintes[5] = (Teinte){"Orange", 230, 130, 50};
    }
    *nb_teintes = 6;
    return teintes;
}

static const char *nom_scancode(SDL_Scancode s)
{
    switch (s) {
    case SDL_SCANCODE_LEFT:
        return "Flèche Gauche";
    case SDL_SCANCODE_RIGHT:
        return "Flèche Droite";
    case SDL_SCANCODE_UP:
        return "Flèche Haut";
    case SDL_SCANCODE_DOWN:
        return "Flèche Bas";
    case SDL_SCANCODE_SPACE:
        return "Espace";
    case SDL_SCANCODE_LSHIFT:
        return "Shift Gauche";
    case SDL_SCANCODE_RSHIFT:
        return "Shift Droit";
    case SDL_SCANCODE_Z:
        return "Z";
    case SDL_SCANCODE_X:
        return "X";
    case SDL_SCANCODE_A:
        return "A";
    case SDL_SCANCODE_D:
        return "D";
    case SDL_SCANCODE_W:
        return "W";
    case SDL_SCANCODE_S:
        return "S";
    case SDL_SCANCODE_E:
        return "E";
    case SDL_SCANCODE_Q:
        return "Q";
    default:
        return "?";
    }
}

static void defaut_config_joueur1(JoueurConfig *c)
{
    int nb_teintes = 0;
    const Teinte *teintes = obtenir_teintes(&nb_teintes);
    if (!teintes || nb_teintes <= 0)
        return;
    c->couleur_r = teintes[0].r;
    c->couleur_g = teintes[0].g;
    c->couleur_b = teintes[0].b;
    c->touche_gauche = SDL_SCANCODE_LEFT;
    c->touche_droite = SDL_SCANCODE_RIGHT;
    c->touche_saut = SDL_SCANCODE_UP;
    c->touche_course = SDL_SCANCODE_RSHIFT;
    c->touche_attaque = SDL_SCANCODE_SPACE;
}

static void defaut_config_joueur2(JoueurConfig *c)
{
    int nb_teintes = 0;
    const Teinte *teintes = obtenir_teintes(&nb_teintes);
    if (!teintes || nb_teintes <= 2)
        return;
    c->couleur_r = teintes[2].r;
    c->couleur_g = teintes[2].g;
    c->couleur_b = teintes[2].b;
    c->touche_gauche = SDL_SCANCODE_A;
    c->touche_droite = SDL_SCANCODE_D;
    c->touche_saut = SDL_SCANCODE_W;
    c->touche_course = SDL_SCANCODE_LSHIFT;
    c->touche_attaque = SDL_SCANCODE_E;
}

static void bouton_rect(SDL_Rect *r, int i, int y0)
{
    r->x = 120 + (i % 2) * 420;
    r->y = y0 + (i / 2) * 52;
    r->w = 380;
    r->h = 44;
}

static int pt_dans_rect(int x, int y, const SDL_Rect *r)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

int menu_personnage(SDL_Renderer *renderer, TTF_Font *font, const char *titre_joueur,
                    JoueurConfig *cfg)
{
    int resultat = 0;
    int nb_teintes = 0;
    const Teinte *teintes = obtenir_teintes(&nb_teintes);
    if (!teintes || nb_teintes <= 0)
        return resultat;

    if (strstr(titre_joueur, "2"))
        defaut_config_joueur2(cfg);
    else
        defaut_config_joueur1(cfg);

    int teinte_idx = 0;
    if (strstr(titre_joueur, "2"))
        teinte_idx = 2;

    enum {
        CHAMP_GAUCHE = 0,
        CHAMP_DROITE,
        CHAMP_SAUT,
        CHAMP_COURSE,
        CHAMP_ATTAQUE,
        CHAMP_NONE
    } en_attente = CHAMP_NONE;

    SDL_Rect btn_teinte_moins = {120, 200, 80, 40};
    SDL_Rect btn_teinte_plus = {220, 200, 80, 40};
    SDL_Rect *btn_touches = (SDL_Rect *)malloc(5 * sizeof(SDL_Rect));
    const char **labels = NULL;
    SDL_Scancode **scancodes = NULL;
    char *ligne = NULL;
    if (!btn_touches)
        return resultat;
    for (int i = 0; i < 5; i++)
        bouton_rect(&btn_touches[i], i, 280);
    labels = (const char **)malloc(5 * sizeof(const char *));
    scancodes = (SDL_Scancode **)malloc(5 * sizeof(SDL_Scancode *));
    if (!labels || !scancodes) {
        goto cleanup;
    }
    labels[0] = "Gauche";
    labels[1] = "Droite";
    labels[2] = "Saut";
    labels[3] = "Course";
    labels[4] = "Attaque";
    scancodes[0] = &cfg->touche_gauche;
    scancodes[1] = &cfg->touche_droite;
    scancodes[2] = &cfg->touche_saut;
    scancodes[3] = &cfg->touche_course;
    scancodes[4] = &cfg->touche_attaque;


    SDL_Rect btn_jouer = {500, 620, 260, 50};
    SDL_Rect btn_defaut = {120, 620, 200, 50};

    int running = 1;
    SDL_Event e;
    SDL_Color blanc = {255, 255, 255, 255};
    SDL_Color jaune = {255, 230, 120, 255};
    ligne = (char *)malloc(256);
    if (!ligne) {
        goto cleanup;
    }

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
            {
                goto cleanup;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (en_attente != CHAMP_NONE)
                    en_attente = CHAMP_NONE;
                else
                {
                    goto cleanup;
                }
            }

            if (en_attente != CHAMP_NONE && e.type == SDL_KEYDOWN) {
                SDL_Scancode sc = e.key.keysym.scancode;
                if (sc != SDL_SCANCODE_ESCAPE) {
                    switch (en_attente) {
                    case CHAMP_GAUCHE:
                        cfg->touche_gauche = sc;
                        break;
                    case CHAMP_DROITE:
                        cfg->touche_droite = sc;
                        break;
                    case CHAMP_SAUT:
                        cfg->touche_saut = sc;
                        break;
                    case CHAMP_COURSE:
                        cfg->touche_course = sc;
                        break;
                    case CHAMP_ATTAQUE:
                        cfg->touche_attaque = sc;
                        break;
                    default:
                        break;
                    }
                }
                en_attente = CHAMP_NONE;
                continue;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                if (pt_dans_rect(mx, my, &btn_teinte_moins)) {
                    teinte_idx = (teinte_idx - 1 + nb_teintes) % nb_teintes;
                    cfg->couleur_r = teintes[teinte_idx].r;
                    cfg->couleur_g = teintes[teinte_idx].g;
                    cfg->couleur_b = teintes[teinte_idx].b;
                }
                if (pt_dans_rect(mx, my, &btn_teinte_plus)) {
                    teinte_idx = (teinte_idx + 1) % nb_teintes;
                    cfg->couleur_r = teintes[teinte_idx].r;
                    cfg->couleur_g = teintes[teinte_idx].g;
                    cfg->couleur_b = teintes[teinte_idx].b;
                }

                if (pt_dans_rect(mx, my, &btn_touches[0]))
                    en_attente = CHAMP_GAUCHE;
                else if (pt_dans_rect(mx, my, &btn_touches[1]))
                    en_attente = CHAMP_DROITE;
                else if (pt_dans_rect(mx, my, &btn_touches[2]))
                    en_attente = CHAMP_SAUT;
                else if (pt_dans_rect(mx, my, &btn_touches[3]))
                    en_attente = CHAMP_COURSE;
                else if (pt_dans_rect(mx, my, &btn_touches[4]))
                    en_attente = CHAMP_ATTAQUE;

                if (pt_dans_rect(mx, my, &btn_defaut)) {
                    if (strstr(titre_joueur, "2"))
                        defaut_config_joueur2(cfg);
                    else
                        defaut_config_joueur1(cfg);
                    teinte_idx = strstr(titre_joueur, "2") ? 2 : 0;
                }

                if (pt_dans_rect(mx, my, &btn_jouer))
                {
                    resultat = 1;
                    goto cleanup;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 28, 32, 48, 255);
        SDL_RenderClear(renderer);

        snprintf(ligne, 256, "%s — Choix perso & tenue & touches", titre_joueur);
        SDL_Surface *st = TTF_RenderUTF8_Blended(font, ligne, blanc);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {120, 40, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }

        snprintf(ligne, 256, "Tenue : %s  (<  >)", teintes[teinte_idx].nom);
        st = TTF_RenderUTF8_Blended(font, ligne, jaune);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {320, 200, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }

        SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
        SDL_RenderFillRect(renderer, &btn_teinte_moins);
        SDL_RenderFillRect(renderer, &btn_teinte_plus);
        st = TTF_RenderUTF8_Blended(font, "-", blanc);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {btn_teinte_moins.x + 32, btn_teinte_moins.y + 8, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }
        st = TTF_RenderUTF8_Blended(font, "+", blanc);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {btn_teinte_plus.x + 32, btn_teinte_plus.y + 8, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }

        for (int i = 0; i < 5; i++) {
            SDL_SetRenderDrawColor(renderer, 60, 70, 100, 255);
            if ((int)en_attente == i)
                SDL_SetRenderDrawColor(renderer, 120, 90, 40, 255);
            SDL_RenderFillRect(renderer, &btn_touches[i]);

            snprintf(ligne, 256, "%s : %s", labels[i], nom_scancode(*scancodes[i]));
            st = TTF_RenderUTF8_Blended(font, ligne, blanc);
            if (st) {
                SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
                SDL_Rect rp = {btn_touches[i].x + 12, btn_touches[i].y + 10, st->w, st->h};
                SDL_RenderCopy(renderer, tt, NULL, &rp);
                SDL_FreeSurface(st);
                SDL_DestroyTexture(tt);
            }
        }

        SDL_SetRenderDrawColor(renderer, 50, 120, 80, 255);
        SDL_RenderFillRect(renderer, &btn_jouer);
        st = TTF_RenderUTF8_Blended(font, "Valider et continuer", blanc);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {btn_jouer.x + 20, btn_jouer.y + 12, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }

        SDL_SetRenderDrawColor(renderer, 70, 70, 90, 255);
        SDL_RenderFillRect(renderer, &btn_defaut);
        st = TTF_RenderUTF8_Blended(font, "Touches par défaut", blanc);
        if (st) {
            SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
            SDL_Rect rp = {btn_defaut.x + 10, btn_defaut.y + 12, st->w, st->h};
            SDL_RenderCopy(renderer, tt, NULL, &rp);
            SDL_FreeSurface(st);
            SDL_DestroyTexture(tt);
        }

        if (en_attente != CHAMP_NONE) {
            st = TTF_RenderUTF8_Blended(font, "Appuyez sur une touche… (Échap annule)", jaune);
            if (st) {
                SDL_Texture *tt = SDL_CreateTextureFromSurface(renderer, st);
                SDL_Rect rp = {120, 520, st->w, st->h};
                SDL_RenderCopy(renderer, tt, NULL, &rp);
                SDL_FreeSurface(st);
                SDL_DestroyTexture(tt);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

cleanup:
    free(btn_touches);
    free(labels);
    free(scancodes);
    free(ligne);
    return resultat;
}
