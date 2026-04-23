#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meilleurs_scores.h"

#define SCORES_MAX_LIGNES 64
#define SCORES_AFFICHES 10
#define DUREE_CLIC_BOUTON_MS 140u
#define NOM_TAILLE 50

typedef struct {
    char *nom;
    int score;
} LigneScore;

static int char_autorise(char c)
{
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    if (c == ' ' || c == '_' || c == '-')
        return 1;
    return 0;
}

static int comparer_scores_desc(const void *a, const void *b)
{
    const LigneScore *la = (const LigneScore *)a;
    const LigneScore *lb = (const LigneScore *)b;
    if (lb->score != la->score)
        return lb->score - la->score;
    const char *na = la->nom ? la->nom : "";
    const char *nb = lb->nom ? lb->nom : "";
    return strcmp(na, nb);
}

static int charger_scores_tries(LigneScore *out, int max)
{
    FILE *f = fopen("scores.txt", "r");
    if (!f)
        return 0;
    int n = 0;
    char *ligne = (char *)malloc(256);
    if (!ligne) {
        fclose(f);
        return 0;
    }
    while (n < max && fgets(ligne, 256, f)) {
        char *nl = strchr(ligne, '\n');
        if (nl)
            *nl = '\0';
        char *last_sp = strrchr(ligne, ' ');
        if (!last_sp || last_sp == ligne)
            continue;
        *last_sp = '\0';
        int sc = atoi(last_sp + 1);
        size_t lnom = strlen(ligne);
        out[n].nom = (char *)malloc(lnom + 1);
        if (!out[n].nom)
            continue;
        memcpy(out[n].nom, ligne, lnom + 1);
        out[n].score = sc;
        n++;
    }
    free(ligne);
    fclose(f);
    if (n > 1)
        qsort(out, (size_t)n, sizeof(LigneScore), comparer_scores_desc);
    return n;
}

static void texte_centre(SDL_Renderer *r, TTF_Font *font, const char *texte, SDL_Color couleur,
                         int cx, int y)
{
    if (!font || !texte)
        return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, texte, couleur);
    if (!s)
        return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (!t)
        return;
    int w = 0, h = 0;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_Rect dst = {cx - w / 2, y, w, h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t);
}

static void panneau_centre(SDL_Renderer *r, int rw, int rh, int marge)
{
    SDL_Rect fond = {marge, marge, rw - 2 * marge, rh - 2 * marge};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    /* Laisser le bgmenu bien visible: pas de voile sombre plein panneau. */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 70);
    SDL_RenderDrawRect(r, &fond);
    SDL_SetRenderDrawColor(r, 120, 160, 235, 180);
    SDL_RenderDrawRect(r, &fond);
    SDL_Rect bord = {fond.x + 2, fond.y + 2, fond.w - 4, fond.h - 4};
    SDL_SetRenderDrawColor(r, 255, 255, 255, 45);
    SDL_RenderDrawRect(r, &bord);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static int souris_dans_rect(int x, int y, const SDL_Rect *rect)
{
    return x >= rect->x && x < rect->x + rect->w && y >= rect->y && y < rect->y + rect->h;
}

static void ajuster_rect_texture(SDL_Texture *tex, SDL_Rect *dst, int max_w, int max_h)
{
    if (!tex) {
        dst->w = max_w;
        dst->h = max_h;
        return;
    }
    int tw = 0, th = 0;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    if (tw <= 0 || th <= 0) {
        dst->w = max_w;
        dst->h = max_h;
        return;
    }
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

static SDL_Texture *charger_texture_multi(SDL_Renderer *renderer, const char **paths, int count)
{
    for (int i = 0; i < count; i++) {
        SDL_Texture *t = IMG_LoadTexture(renderer, paths[i]);
        if (t)
            return t;
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

void sous_menu_meilleurs_scores(SDL_Renderer *renderer)
{
    int rw = 1280, rh = 720;
    SDL_GetRendererOutputSize(renderer, &rw, &rh);

    SDL_Event event;
    int running = 1;
    int etat_saisie = 1;
    char *nom = (char *)calloc(NOM_TAILLE, sizeof(char));
    if (!nom)
        return;
    Uint32 ticks_debut = SDL_GetTicks();
    int curseur_visible = 1;

    const char **bg_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!bg_paths) {
        free(nom);
        return;
    }
    bg_paths[0] = "assets/images/bgmenu.png";
    bg_paths[1] = "./assets/images/bgmenu.png";
    bg_paths[2] = "../assets/images/bgmenu.png";
    bg_paths[3] = "jeu/assets/images/bgmenu.png";
    SDL_Texture *background = charger_texture_multi(renderer, bg_paths, 4);
    free(bg_paths);

    const char **font_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!font_paths) {
        if (background)
            SDL_DestroyTexture(background);
        free(nom);
        return;
    }
    font_paths[0] = "assets/fonts/font.ttf";
    font_paths[1] = "./assets/fonts/font.ttf";
    font_paths[2] = "../assets/fonts/font.ttf";
    font_paths[3] = "jeu/assets/fonts/font.ttf";
    TTF_Font *font = ouvrir_font_multi(font_paths, 4, 26);
    if (!font) {
        free(font_paths);
        if (background)
            SDL_DestroyTexture(background);
        free(nom);
        return;
    }
    TTF_Font *font_titre = ouvrir_font_multi(font_paths, 4, 42);
    free(font_paths);
    if (!font_titre)
        font_titre = font;

    SDL_Color blanc = {248, 250, 255, 255};
    SDL_Color gris = {170, 180, 200, 255};
    SDL_Color or_ = {255, 210, 90, 255};
    SDL_Color argent = {210, 220, 235, 255};
    SDL_Color bronze = {205, 145, 90, 255};

    Mix_Chunk *hover = Mix_LoadWAV("assets/sounds/hover.wav");
    if (!hover)
        hover = Mix_LoadWAV("jeu/assets/sounds/hover.wav");
    Mix_Music *victory = Mix_LoadMUS("assets/sounds/victory.mp3");
    if (!victory)
        victory = Mix_LoadMUS("jeu/assets/sounds/victory.mp3");

    const char **valider_paths = (const char **)malloc(4 * sizeof(const char *));
    const char **retour_paths = (const char **)malloc(4 * sizeof(const char *));
    const char **quitter_paths = (const char **)malloc(4 * sizeof(const char *));
    if (!valider_paths || !retour_paths || !quitter_paths) {
        free(valider_paths);
        free(retour_paths);
        free(quitter_paths);
        if (hover)
            Mix_FreeChunk(hover);
        if (victory)
            Mix_FreeMusic(victory);
        if (background)
            SDL_DestroyTexture(background);
        if (font)
            TTF_CloseFont(font);
        if (font_titre && font_titre != font)
            TTF_CloseFont(font_titre);
        free(nom);
        return;
    }
    valider_paths[0] = "assets/images/valider btn.png";
    valider_paths[1] = "./assets/images/valider btn.png";
    valider_paths[2] = "../assets/images/valider btn.png";
    valider_paths[3] = "jeu/assets/images/valider btn.png";
    retour_paths[0] = "assets/images/retour btn.png";
    retour_paths[1] = "./assets/images/retour btn.png";
    retour_paths[2] = "../assets/images/retour btn.png";
    retour_paths[3] = "jeu/assets/images/retour btn.png";
    quitter_paths[0] = "assets/images/quitter btn.png";
    quitter_paths[1] = "./assets/images/quitter btn.png";
    quitter_paths[2] = "../assets/images/quitter btn.png";
    quitter_paths[3] = "jeu/assets/images/quitter btn.png";
    SDL_Texture *btnValiderTex = charger_texture_multi(renderer, valider_paths, 4);
    SDL_Texture *btnRetourTex = charger_texture_multi(renderer, retour_paths, 4);
    SDL_Texture *btnQuitterTex = charger_texture_multi(renderer, quitter_paths, 4);
    free(valider_paths);
    free(retour_paths);
    free(quitter_paths);

    int marge = rw > 900 ? 72 : 36;
    int cx = rw / 2;
    int base_y = rh / 2 - 120;

    SDL_Rect zone_valider = {cx - 155, base_y + 228, 310, 84};
    SDL_Rect zone_nom = {cx - 300, base_y + 34, 600, 108};
    SDL_Rect zone_retour = {cx - 250, base_y + 326, 220, 74};
    SDL_Rect zone_quitter = {cx + 30, base_y + 326, 220, 74};

    enum { SURVOL_RIEN = -1, SURVOL_VALIDER = 0, SURVOL_RETOUR, SURVOL_QUITTER };
    int survol_actuel = SURVOL_RIEN;
    int dernier_survol = SURVOL_RIEN;
    int presse_bouton = SURVOL_RIEN;
    Uint32 fin_effet_clic_ms = 0;
    int action_en_attente = SURVOL_RIEN;

    SDL_StartTextInput();

    while (running) {
        Uint32 maintenant = SDL_GetTicks();
        if ((maintenant - ticks_debut) > 530) {
            ticks_debut = maintenant;
            curseur_visible = !curseur_visible;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (etat_saisie)
                        running = 0;
                    else {
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                        etat_saisie = 1;
                    }
                }
            }

            if (etat_saisie) {
                if (event.type == SDL_TEXTINPUT) {
                    size_t ln = strlen(nom);
                    for (int i = 0; event.text.text[i] != '\0'; i++) {
                        char c = event.text.text[i];
                        if (!char_autorise(c))
                            continue;
                        if (ln + 1 >= (size_t)NOM_TAILLE - 1)
                            break;
                        nom[ln++] = c;
                        nom[ln] = '\0';
                    }
                }
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(nom) > 0)
                        nom[strlen(nom) - 1] = '\0';
                    if (event.key.keysym.sym == SDLK_BACKSPACE &&
                        (event.key.keysym.mod & KMOD_CTRL)) {
                        nom[0] = '\0';
                    }
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                        etat_saisie = 0;
                        if (victory)
                            Mix_PlayMusic(victory, 0);
                    }
                }
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x, y = event.button.y;
                    if (souris_dans_rect(x, y, &zone_nom)) {
                        nom[0] = '\0';
                    }
                    if (souris_dans_rect(x, y, &zone_valider)) {
                        presse_bouton = SURVOL_VALIDER;
                        fin_effet_clic_ms = SDL_GetTicks() + DUREE_CLIC_BOUTON_MS;
                        action_en_attente = SURVOL_VALIDER;
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                    }
                }
            } else {
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                        etat_saisie = 1;
                    }
                }
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x, y = event.button.y;
                    if (souris_dans_rect(x, y, &zone_retour)) {
                        presse_bouton = SURVOL_RETOUR;
                        fin_effet_clic_ms = SDL_GetTicks() + DUREE_CLIC_BOUTON_MS;
                        action_en_attente = SURVOL_RETOUR;
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                    }
                    if (souris_dans_rect(x, y, &zone_quitter)) {
                        presse_bouton = SURVOL_QUITTER;
                        fin_effet_clic_ms = SDL_GetTicks() + DUREE_CLIC_BOUTON_MS;
                        action_en_attente = SURVOL_QUITTER;
                        if (hover)
                            Mix_PlayChannel(-1, hover, 0);
                    }
                }
            }
        }
        if (presse_bouton != SURVOL_RIEN && SDL_GetTicks() >= fin_effet_clic_ms) {
            presse_bouton = SURVOL_RIEN;
            if (action_en_attente == SURVOL_VALIDER) {
                etat_saisie = 0;
                if (victory)
                    Mix_PlayMusic(victory, 0);
            } else if (action_en_attente == SURVOL_RETOUR) {
                etat_saisie = 1;
            } else if (action_en_attente == SURVOL_QUITTER) {
                running = 0;
            }
            action_en_attente = SURVOL_RIEN;
        }

        if (etat_saisie) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            int nv = souris_dans_rect(x, y, &zone_valider) ? SURVOL_VALIDER : SURVOL_RIEN;
            if (nv != dernier_survol && nv != SURVOL_RIEN && hover)
                Mix_PlayChannel(-1, hover, 0);
            dernier_survol = nv;
            survol_actuel = nv;
        } else {
            int x, y;
            SDL_GetMouseState(&x, &y);
            int nv = SURVOL_RIEN;
            if (souris_dans_rect(x, y, &zone_retour))
                nv = SURVOL_RETOUR;
            else if (souris_dans_rect(x, y, &zone_quitter))
                nv = SURVOL_QUITTER;
            if (nv != dernier_survol && nv != SURVOL_RIEN && hover)
                Mix_PlayChannel(-1, hover, 0);
            dernier_survol = nv;
            survol_actuel = nv;
        }

        SDL_RenderClear(renderer);
        if (background) {
            SDL_Rect bg_dst = {0, 0, rw, rh};
            SDL_RenderCopy(renderer, background, NULL, &bg_dst);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 25, 30, 45, 255);
            SDL_RenderFillRect(renderer, NULL);
        }

        panneau_centre(renderer, rw, rh, marge);

        if (etat_saisie) {
            SDL_Texture *titre_tex = NULL;
            int tw = 0, th = 0;
            if (font_titre) {
                SDL_Surface *st = TTF_RenderUTF8_Blended(font_titre, "Meilleurs scores", blanc);
                if (st) {
                    titre_tex = SDL_CreateTextureFromSurface(renderer, st);
                    tw = st->w;
                    th = st->h;
                    SDL_FreeSurface(st);
                }
            }
            if (titre_tex) {
                SDL_Rect tp = {cx - tw / 2, marge + 48, tw, th};
                SDL_RenderCopy(renderer, titre_tex, NULL, &tp);
                SDL_DestroyTexture(titre_tex);
            }

            texte_centre(renderer, font,
                         "Enregistrez un nom pour la partie (visible dans le classement).", gris, cx,
                         marge + 48 + th + 16);
            texte_centre(renderer, font, "Votre nom ou pseudo", blanc, cx, base_y + 12);

            char *affichage = (char *)malloc(NOM_TAILLE + 8);
            if (!affichage)
                affichage = nom;
            if (strlen(nom) == 0)
                snprintf(affichage, NOM_TAILLE + 8, curseur_visible ? "|" : " ");
            else if (curseur_visible)
                snprintf(affichage, NOM_TAILLE + 8, "%s|", nom);
            else
                snprintf(affichage, NOM_TAILLE + 8, "%s", nom);
            texte_centre(renderer, font, affichage, blanc, cx, base_y + 78);
            if (affichage != nom)
                free(affichage);

            SDL_SetRenderDrawColor(renderer, 200, 210, 235, 255);
            SDL_Rect ligne_nom = {cx - 280, base_y + 110, 560, 2};
            SDL_RenderFillRect(renderer, &ligne_nom);

            texte_centre(renderer, font,
                         "Clic sur la zone : effacer   ·   Ctrl+Backspace : effacer   ·   Entree : classement   ·   Echap : fermer",
                         gris, cx, base_y + 152);

            SDL_Rect dst_val = zone_valider;
            if (btnValiderTex) {
                ajuster_rect_texture(btnValiderTex, &dst_val, zone_valider.w, zone_valider.h);
                if (presse_bouton == SURVOL_VALIDER)
                    dst_val = rect_scale_centered(dst_val, 1.12f);
                SDL_RenderCopy(renderer, btnValiderTex, NULL, &dst_val);
            } else {
                SDL_SetRenderDrawColor(renderer, 70, 130, 95, 255);
                SDL_RenderFillRect(renderer, &zone_valider);
                texte_centre(renderer, font, "Voir le classement", blanc,
                             zone_valider.x + zone_valider.w / 2, zone_valider.y + 25);
            }
            if (survol_actuel == SURVOL_VALIDER) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 90);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_RenderDrawRect(renderer, &zone_valider);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        } else {
            LigneScore *tab = (LigneScore *)calloc(SCORES_MAX_LIGNES, sizeof(LigneScore));
            int nb = tab ? charger_scores_tries(tab, SCORES_MAX_LIGNES) : 0;

            SDL_Texture *titre_tex = NULL;
            int tw = 0, th = 0;
            if (font_titre) {
                SDL_Surface *st = TTF_RenderUTF8_Blended(font_titre, "Classement", blanc);
                if (st) {
                    titre_tex = SDL_CreateTextureFromSurface(renderer, st);
                    tw = st->w;
                    th = st->h;
                    SDL_FreeSurface(st);
                }
            }
            if (titre_tex) {
                SDL_Rect tp = {cx - tw / 2, marge + 40, tw, th};
                SDL_RenderCopy(renderer, titre_tex, NULL, &tp);
                SDL_DestroyTexture(titre_tex);
            }

            char *sous = (char *)malloc(160);
            if (!sous)
                sous = nom;
            if (strlen(nom) > 0)
                snprintf(sous, 160, "Joueur : %s", nom);
            else
                snprintf(sous, 160, "Joueur : (anonyme)");
            texte_centre(renderer, font, sous, gris, cx, marge + 40 + th + 8);
            if (sous != nom)
                free(sous);

            int y_liste = marge + 40 + th + 72;
            int lim = nb < SCORES_AFFICHES ? nb : SCORES_AFFICHES;

            if (lim == 0) {
                texte_centre(renderer, font, "Aucun score enregistré pour le moment.", gris, cx,
                             y_liste + 40);
            } else {
                for (int i = 0; i < lim; i++) {
                    char *ligne = (char *)malloc(128);
                    if (!ligne)
                        continue;
                    snprintf(ligne, 128, "%2d.  %.24s  %6d pts", i + 1,
                             tab[i].nom ? tab[i].nom : "",
                             tab[i].score);
                    SDL_Color c = blanc;
                    if (i == 0)
                        c = or_;
                    else if (i == 1)
                        c = argent;
                    else if (i == 2)
                        c = bronze;
                    texte_centre(renderer, font, ligne, c, cx, y_liste + i * 34);
                    free(ligne);
                }
            }
            for (int i = 0; i < nb; i++)
                free(tab[i].nom);
            free(tab);

            texte_centre(renderer, font,
                         "Entrée : modifier le nom   ·   Échap : retour   ·   Souris : boutons", gris, cx,
                         rh - marge - 56);

            SDL_Rect dst_ret = zone_retour;
            SDL_Rect dst_quit = zone_quitter;
            if (btnRetourTex) {
                ajuster_rect_texture(btnRetourTex, &dst_ret, zone_retour.w, zone_retour.h);
                if (presse_bouton == SURVOL_RETOUR)
                    dst_ret = rect_scale_centered(dst_ret, 1.12f);
                SDL_RenderCopy(renderer, btnRetourTex, NULL, &dst_ret);
            } else {
                SDL_SetRenderDrawColor(renderer, 80, 95, 130, 255);
                SDL_RenderFillRect(renderer, &zone_retour);
                texte_centre(renderer, font, "Retour", blanc, zone_retour.x + zone_retour.w / 2,
                             zone_retour.y + 20);
            }
            if (btnQuitterTex) {
                ajuster_rect_texture(btnQuitterTex, &dst_quit, zone_quitter.w, zone_quitter.h);
                if (presse_bouton == SURVOL_QUITTER)
                    dst_quit = rect_scale_centered(dst_quit, 1.12f);
                SDL_RenderCopy(renderer, btnQuitterTex, NULL, &dst_quit);
            } else {
                SDL_SetRenderDrawColor(renderer, 120, 65, 65, 255);
                SDL_RenderFillRect(renderer, &zone_quitter);
                texte_centre(renderer, font, "Fermer", blanc, zone_quitter.x + zone_quitter.w / 2,
                             zone_quitter.y + 20);
            }

            if (survol_actuel == SURVOL_RETOUR || survol_actuel == SURVOL_QUITTER) {
                SDL_Rect *hi = survol_actuel == SURVOL_RETOUR ? &zone_retour : &zone_quitter;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_RenderDrawRect(renderer, hi);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    if (hover)
        Mix_FreeChunk(hover);
    if (victory)
        Mix_FreeMusic(victory);
    if (btnValiderTex)
        SDL_DestroyTexture(btnValiderTex);
    if (btnRetourTex)
        SDL_DestroyTexture(btnRetourTex);
    if (btnQuitterTex)
        SDL_DestroyTexture(btnQuitterTex);
    if (background)
        SDL_DestroyTexture(background);
    if (font)
        TTF_CloseFont(font);
    if (font_titre && font_titre != font)
        TTF_CloseFont(font_titre);
    free(nom);
}
