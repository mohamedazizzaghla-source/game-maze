#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include "meilleurs_scores.h"
#include "joueur.h"
#include "menu_personnage.h"

#define WIDTH 1280
#define HEIGHT 720
#define SOL_Y (HEIGHT - 72)

typedef enum {
    ECRAN_MENU = 0,
    ECRAN_JEU_1P,
    ECRAN_JEU_2P
} EcranPrincipal;

static SDL_Surface *charger_surface_multi(const char **paths, int count)
{
    for (int i = 0; i < count; i++) {
        SDL_Surface *s = IMG_Load(paths[i]);
        if (s)
            return s;
    }
    return NULL;
}

static TTF_Font *ouvrir_font_multi(const char **paths, int count, int ptsize)
{
    for (int i = 0; i < count; i++) {
        TTF_Font *f = TTF_OpenFont(paths[i], ptsize);
        if (f)
            return f;
    }
    return NULL;
}

static SDL_Texture *charger_texture_multi(SDL_Renderer *renderer, const char **paths, int count)
{
    for (int i = 0; i < count; i++) {
        SDL_Texture *t = IMG_LoadTexture(renderer, paths[i]);
        if (t)
            return t;
    }
    return NULL;
}

static void ajuster_rect_texture(SDL_Texture *tex, SDL_Rect *dst, int max_w, int max_h)
{
    if (!tex)
        return;
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    if (tw <= 0 || th <= 0)
        return;
    float sx = (float)max_w / (float)tw;
    float sy = (float)max_h / (float)th;
    float s = sx < sy ? sx : sy;
    dst->w = (int)(tw * s);
    dst->h = (int)(th * s);
    dst->x += (max_w - dst->w) / 2;
    dst->y += (max_h - dst->h) / 2;
}

static SDL_Rect rect_scale_centered(SDL_Rect r, float scale)
{
    if (scale <= 0.0f)
        return r;
    int nw = (int)(r.w * scale);
    int nh = (int)(r.h * scale);
    SDL_Rect out = {r.x - (nw - r.w) / 2, r.y - (nh - r.h) / 2, nw, nh};
    return out;
}

static int point_dans_rect(int x, int y, const SDL_Rect *r)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static int menu_principal(SDL_Renderer *renderer, TTF_Font *font, EcranPrincipal *choix)
{
    enum {
        BTN_J1 = 0,
        BTN_J2,
        BTN_SCORES,
        BTN_QUITTER,
        BTN_TOTAL
    };
    const Uint32 DELAI_CLIC_MS = 140u;

    int rw = WIDTH, rh = HEIGHT;
    SDL_GetRendererOutputSize(renderer, &rw, &rh);
    int cx = rw / 2;
    int btn_w = rw > 1200 ? 360 : 320;
    int btn_h = rw > 1200 ? 92 : 82;
    int y0 = rh / 2 - 170;
    int gap = 96;

    SDL_Rect *boutons = (SDL_Rect *)malloc(BTN_TOTAL * sizeof(SDL_Rect));
    if (!boutons)
        return 0;
    for (int i = 0; i < BTN_TOTAL; i++)
        boutons[i] = (SDL_Rect){cx - btn_w / 2, y0 + i * gap, btn_w, btn_h};

    const char **bg_paths = (const char **)malloc(4 * sizeof(const char *));
    const char **play_paths = (const char **)malloc(4 * sizeof(const char *));
    const char **quit_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!bg_paths || !play_paths || !quit_paths) {
        free(boutons);
        free(bg_paths);
        free(play_paths);
        free(quit_paths);
        return 0;
    }
    bg_paths[0] = "assets/images/bgmenu.png";
    bg_paths[1] = "./assets/images/bgmenu.png";
    bg_paths[2] = "../assets/images/bgmenu.png";
    bg_paths[3] = "jeu/assets/images/bgmenu.png";
    play_paths[0] = "assets/images/play btn.png";
    play_paths[1] = "./assets/images/play btn.png";
    play_paths[2] = "../assets/images/play btn.png";
    play_paths[3] = "jeu/assets/images/play btn.png";
    quit_paths[0] = "assets/images/quitter btn.png";
    quit_paths[1] = "./assets/images/quitter btn.png";
    quit_paths[2] = "../assets/images/quitter btn.png";
    quit_paths[3] = "jeu/assets/images/quitter btn.png";
    SDL_Texture *bg_tex = charger_texture_multi(renderer, bg_paths, 4);
    SDL_Texture *play_tex = charger_texture_multi(renderer, play_paths, 4);
    SDL_Texture *quit_tex = charger_texture_multi(renderer, quit_paths, 4);
    free(bg_paths);
    free(play_paths);
    free(quit_paths);

    int run = 1;
    int quitter = 0;
    int survol = -1;
    int presse = -1;
    int action = -1;
    Uint32 fin_clic_ms = 0;
    SDL_Event ev;
    SDL_Color blanc = {255, 255, 255, 255};
    SDL_Color sombre = {20, 25, 35, 255};

    while (run) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
            {
                quitter = 1;
                run = 0;
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int x = ev.button.x, y = ev.button.y;
                for (int i = 0; i < BTN_TOTAL; i++) {
                    if (point_dans_rect(x, y, &boutons[i])) {
                        presse = i;
                        action = i;
                        fin_clic_ms = SDL_GetTicks() + DELAI_CLIC_MS;
                        break;
                    }
                }
            }
        }

        SDL_RenderClear(renderer);
        if (bg_tex) {
            SDL_Rect bg_dst = {0, 0, rw, rh};
            SDL_RenderCopy(renderer, bg_tex, NULL, &bg_dst);
        } else {
            SDL_SetRenderDrawColor(renderer, 20, 24, 36, 255);
            SDL_RenderFillRect(renderer, NULL);
        }

        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        survol = -1;
        for (int i = 0; i < BTN_TOTAL; i++) {
            if (point_dans_rect(mx, my, &boutons[i])) {
                survol = i;
                break;
            }
        }

        const char **textes = (const char **)malloc(BTN_TOTAL * sizeof(const char *));
        if (!textes) {
            SDL_DestroyTexture(bg_tex);
            SDL_DestroyTexture(play_tex);
            SDL_DestroyTexture(quit_tex);
            free(boutons);
            return 0;
        }
        textes[0] = "1 joueur";
        textes[1] = "2 joueurs (même clavier)";
        textes[2] = "Meilleurs scores";
        textes[3] = "Quitter";
        for (int i = 0; i < BTN_TOTAL; i++) {
            SDL_Texture *btn_tex = (i == BTN_QUITTER) ? quit_tex : play_tex;
            SDL_Rect dst = boutons[i];
            if (btn_tex) {
                ajuster_rect_texture(btn_tex, &dst, boutons[i].w, boutons[i].h);
                if (presse == i)
                    dst = rect_scale_centered(dst, 1.12f);
                SDL_RenderCopy(renderer, btn_tex, NULL, &dst);
            } else {
                SDL_SetRenderDrawColor(renderer, i == BTN_QUITTER ? 120 : 55,
                                       i == BTN_QUITTER ? 65 : 65, i == BTN_QUITTER ? 65 : 95, 255);
                SDL_RenderFillRect(renderer, &boutons[i]);
            }

            if (survol == i) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 70);
                SDL_RenderDrawRect(renderer, &boutons[i]);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }

            SDL_Surface *s = TTF_RenderUTF8_Blended(font, textes[i], blanc);
            if (s) {
                SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {boutons[i].x + (boutons[i].w - s->w) / 2,
                              boutons[i].y + (boutons[i].h - s->h) / 2, s->w, s->h};
                SDL_SetTextureColorMod(t, sombre.r, sombre.g, sombre.b);
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s);
                SDL_DestroyTexture(t);
            }
        }
        free(textes);

        if (action != -1 && SDL_GetTicks() >= fin_clic_ms) {
            presse = -1;
            if (action == BTN_J1) {
                *choix = ECRAN_JEU_1P;
                run = 0;
            } else if (action == BTN_J2) {
                *choix = ECRAN_JEU_2P;
                run = 0;
            } else if (action == BTN_SCORES) {
                sous_menu_meilleurs_scores(renderer);
            } else if (action == BTN_QUITTER) {
                SDL_DestroyTexture(bg_tex);
                SDL_DestroyTexture(play_tex);
                SDL_DestroyTexture(quit_tex);
                free(boutons);
                return 0;
            }
            action = -1;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_DestroyTexture(bg_tex);
    SDL_DestroyTexture(play_tex);
    SDL_DestroyTexture(quit_tex);
    free(boutons);
    if (quitter)
        return 0;
    return 1;
}

static void boucle_jeu(SDL_Renderer *renderer, TTF_Font *font, int deux_joueurs)
{
    JoueurConfig cfg1, cfg2;
    if (!menu_personnage(renderer, font, "Joueur 1", &cfg1))
        return;
    if (deux_joueurs) {
        if (!menu_personnage(renderer, font, "Joueur 2", &cfg2))
            return;
    }

    Joueur j1, j2;
    joueur_initialiser(&j1, renderer, 120.0f, (float)(SOL_Y - JOUEUR_FRAME_H), 1, &cfg1);
    if (deux_joueurs)
        joueur_initialiser_joueur2(&j2, renderer, 720.0f, (float)(SOL_Y - JOUEUR_FRAME_H),
                                   &cfg2);

    SDL_Texture *bg = NULL;
    const char **bg_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!bg_paths)
        return;
    bg_paths[0] = "assets/images/background3.png";
    bg_paths[1] = "./assets/images/background3.png";
    bg_paths[2] = "../assets/images/background3.png";
    bg_paths[3] = "jeu/assets/images/background3.png";
    SDL_Surface *surf_bg = charger_surface_multi(bg_paths, 4);
    free(bg_paths);
    if (surf_bg) {
        bg = SDL_CreateTextureFromSurface(renderer, surf_bg);
        SDL_FreeSurface(surf_bg);
    }

    int running = 1;
    SDL_Event event;
    Uint32 ticks_prev = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (float)(now - ticks_prev);
        ticks_prev = now;
        if (dt > 100.0f)
            dt = 100.0f;

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = 0;
                if (event.key.keysym.sym == SDLK_F1)
                    joueur_toucher_ennemi_placeholder(&j1);
                if (event.key.keysym.sym == SDLK_F2)
                    joueur_ramasser_bonus_placeholder(&j1);
                if (deux_joueurs) {
                    if (event.key.keysym.sym == SDLK_F3)
                        joueur_toucher_ennemi_placeholder(&j2);
                    if (event.key.keysym.sym == SDLK_F4)
                        joueur_ramasser_bonus_placeholder(&j2);
                }
            }
        }

        joueur_deplacer(&j1, keys, dt, SOL_Y);
        joueur_animer(&j1, now);
        if (deux_joueurs) {
            joueur_deplacer(&j2, keys, dt, SOL_Y);
            joueur_animer(&j2, now);
        }

        SDL_RenderClear(renderer);
        if (bg) {
            SDL_RenderCopy(renderer, bg, NULL, NULL);
        } else {
            SDL_SetRenderDrawColor(renderer, 40, 55, 72, 255);
            SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, WIDTH, HEIGHT});
        }

        SDL_SetRenderDrawColor(renderer, 34, 40, 52, 255);
        SDL_RenderFillRect(renderer, &(SDL_Rect){0, SOL_Y, WIDTH, HEIGHT - SOL_Y});

        joueur_afficher(renderer, &j1);
        if (deux_joueurs)
            joueur_afficher(renderer, &j2);

        joueur_afficher_hud(renderer, font, &j1, 24, 16);
        if (deux_joueurs)
            joueur_afficher_hud(renderer, font, &j2, 24, 52);

        SDL_Surface *hint = TTF_RenderUTF8_Blended(
            font,
            "Échap : menu principal   |   F1/F2 : test vie/bonus J1   F3/F4 : test J2",
            (SDL_Color){220, 220, 230, 255});
        if (hint) {
            SDL_Texture *th = SDL_CreateTextureFromSurface(renderer, hint);
            SDL_Rect rh = {24, HEIGHT - 36, hint->w, hint->h};
            SDL_RenderCopy(renderer, th, NULL, &rh);
            SDL_FreeSurface(hint);
            SDL_DestroyTexture(th);
        }

        SDL_RenderPresent(renderer);
    }

    joueur_liberer(&j1);
    if (deux_joueurs)
        joueur_liberer(&j2);
    if (bg)
        SDL_DestroyTexture(bg);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL Init error: %s\n", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG Init error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF Init error: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int audio_ok = 0;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
        audio_ok = 1;
    } else {
        /* Environnements sans Pulse/ALSA: fallback silencieux sans bruit console. */
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0)
            audio_ok = 1;
    }

    SDL_Window *window = SDL_CreateWindow("Jeu — Lot 1 Joueur",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                                SDL_RENDERER_ACCELERATED |
                                                    SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        printf("Renderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        if (audio_ok)
            Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    const char **font_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!font_paths) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_ok)
            Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    font_paths[0] = "assets/fonts/font.ttf";
    font_paths[1] = "./assets/fonts/font.ttf";
    font_paths[2] = "../assets/fonts/font.ttf";
    font_paths[3] = "jeu/assets/fonts/font.ttf";
    TTF_Font *font = ouvrir_font_multi(font_paths, 4, 26);
    free(font_paths);
    if (!font) {
        printf("Police introuvable\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_ok)
            Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int app = 1;
    while (app) {
        EcranPrincipal choix = ECRAN_MENU;
        if (!menu_principal(renderer, font, &choix)) {
            app = 0;
            break;
        }
        if (choix == ECRAN_JEU_1P)
            boucle_jeu(renderer, font, 0);
        else if (choix == ECRAN_JEU_2P)
            boucle_jeu(renderer, font, 1);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_ok)
        Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
