#include "joueur.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GRAVITE 2200.0f
#define VITESSE_MARCHE 280.0f
#define VITESSE_COURSE 420.0f
#define IMPULSION_SAUT -620.0f
#define DUREE_ATTAQUE_MS 420u
#define DUREE_BLESSE_MS 420u

/* ---------------- Spritesheet Lot1 ---------------- */

typedef struct {
    SDL_Rect rect;
} FrameRect;

/* Rectangles extraits du PNG `lot1:joueur/spritesheet` (coordonnées en pixels). */
FrameRect *LOT1_IDLE = NULL;
FrameRect *LOT1_RUN = NULL;
FrameRect *LOT1_CLIMB = NULL;
FrameRect *LOT1_ATTACK = NULL;
FrameRect *LOT1_HURT = NULL;
FrameRect *LOT1_DEATH = NULL;

int initialiser_frames_lot1(void)
{
    if (LOT1_IDLE && LOT1_RUN && LOT1_CLIMB && LOT1_ATTACK && LOT1_HURT && LOT1_DEATH)
        return 1;

    LOT1_IDLE = (FrameRect *)malloc(3 * sizeof(FrameRect));
    LOT1_RUN = (FrameRect *)malloc(3 * sizeof(FrameRect));
    LOT1_CLIMB = (FrameRect *)malloc(3 * sizeof(FrameRect));
    LOT1_ATTACK = (FrameRect *)malloc(3 * sizeof(FrameRect));
    LOT1_HURT = (FrameRect *)malloc(3 * sizeof(FrameRect));
    LOT1_DEATH = (FrameRect *)malloc(4 * sizeof(FrameRect));
    if (!LOT1_IDLE || !LOT1_RUN || !LOT1_CLIMB || !LOT1_ATTACK || !LOT1_HURT || !LOT1_DEATH)
        return 0;

    LOT1_IDLE[0] = (FrameRect){{187, 20, 27, 75}};
    LOT1_IDLE[1] = (FrameRect){{259, 20, 18, 75}};
    LOT1_IDLE[2] = (FrameRect){{313, 20, 19, 75}};
    LOT1_RUN[0] = (FrameRect){{188, 104, 27, 71}};
    LOT1_RUN[1] = (FrameRect){{248, 107, 34, 67}};
    LOT1_RUN[2] = (FrameRect){{309, 107, 42, 67}};
    LOT1_CLIMB[0] = (FrameRect){{187, 187, 29, 69}};
    LOT1_CLIMB[1] = (FrameRect){{255, 184, 28, 72}};
    LOT1_CLIMB[2] = (FrameRect){{313, 185, 29, 71}};
    LOT1_ATTACK[0] = (FrameRect){{172, 267, 45, 66}};
    LOT1_ATTACK[1] = (FrameRect){{243, 267, 40, 66}};
    LOT1_ATTACK[2] = (FrameRect){{306, 267, 60, 66}};
    LOT1_HURT[0] = (FrameRect){{188, 344, 33, 65}};
    LOT1_HURT[1] = (FrameRect){{250, 344, 33, 64}};
    LOT1_HURT[2] = (FrameRect){{307, 346, 40, 61}};
    LOT1_DEATH[0] = (FrameRect){{109, 452, 69, 20}};
    LOT1_DEATH[1] = (FrameRect){{192, 447, 60, 25}};
    LOT1_DEATH[2] = (FrameRect){{266, 451, 67, 21}};
    LOT1_DEATH[3] = (FrameRect){{340, 454, 67, 18}};
    return 1;
}

int lot1_anim_nb_frames(JoueurAnim anim)
{
    switch (anim) {
    case J_ANIM_IDLE:
        return 3;
    case J_ANIM_MARCHE:
    case J_ANIM_COURSE:
        return 3;
    case J_ANIM_GRIMPE:
        return 3;
    case J_ANIM_ATTAQUE:
        return 3;
    case J_ANIM_BLESSE:
        return 3;
    case J_ANIM_MORT:
        return 4;
    case J_ANIM_SAUT:
        return 1;
    default:
        return 1;
    }
}

SDL_Rect lot1_anim_frame_rect(JoueurAnim anim, int idx)
{
    if (!initialiser_frames_lot1())
        return (SDL_Rect){0, 0, JOUEUR_FRAME_W, JOUEUR_FRAME_H};
    if (idx < 0)
        idx = 0;

    switch (anim) {
    case J_ANIM_IDLE:
        return LOT1_IDLE[idx % 3].rect;
    case J_ANIM_MARCHE:
    case J_ANIM_COURSE:
        return LOT1_RUN[idx % 3].rect;
    case J_ANIM_GRIMPE:
        return LOT1_CLIMB[idx % 3].rect;
    case J_ANIM_ATTAQUE:
        return LOT1_ATTACK[idx % 3].rect;
    case J_ANIM_BLESSE:
        return LOT1_HURT[idx % 3].rect;
    case J_ANIM_MORT:
        return LOT1_DEATH[idx % 4].rect;
    case J_ANIM_SAUT:
    default:
        /* Pas d'anim saut dédiée dans le lot 1 : on réutilise une frame de run. */
        return LOT1_RUN[1].rect;
    }
}

void lot1_rendre_damier_transparent(SDL_Surface *surf)
{
    if (!surf)
        return;
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA8888, 0);
    if (!rgba)
        return;

    SDL_LockSurface(rgba);
    Uint32 *p = (Uint32 *)rgba->pixels;
    int n = (rgba->pitch / 4) * rgba->h;

    Uint8 r, g, b, a;
    for (int i = 0; i < n; i++) {
        SDL_GetRGBA(p[i], rgba->format, &r, &g, &b, &a);
        /* damier gris (ancienne spritesheet) OU fond très sombre (nouvelle) */
        int is_grey = (abs((int)r - (int)g) <= 3) && (abs((int)g - (int)b) <= 3) &&
                      r >= 90 && r <= 200;
        int is_dark = (r <= 18 && g <= 18 && b <= 18);
        if (is_grey || is_dark) {
            p[i] = SDL_MapRGBA(rgba->format, r, g, b, 0);
        } else {
            p[i] = SDL_MapRGBA(rgba->format, r, g, b, 255);
        }
    }
    SDL_UnlockSurface(rgba);

    SDL_BlitSurface(rgba, NULL, surf, NULL);
    SDL_FreeSurface(rgba);
}

/* ---------------- Sprites procéduraux (fallback) ---------------- */

void dessiner_frame(SDL_Surface *surf, int index_frame, Uint8 cr, Uint8 cg, Uint8 cb,
                    JoueurAnim anim_hint)
{
    int fx = index_frame * JOUEUR_FRAME_W;
    SDL_Rect zone = {fx, 0, JOUEUR_FRAME_W, JOUEUR_FRAME_H};
    Uint32 px = SDL_MapRGB(surf->format, 40, 40, 45);
    SDL_FillRect(surf, &zone, px);
    int dec = 0;
    if (anim_hint == J_ANIM_MARCHE)
        dec = (index_frame % 2) ? 3 : -3;
    if (anim_hint == J_ANIM_COURSE)
        dec = (index_frame % 2) ? 5 : -5;

    SDL_Rect corps = {fx + 10, 14 + dec, 20, 26};
    Uint32 couleur_corps = SDL_MapRGB(surf->format, cr, cg, cb);
    SDL_FillRect(surf, &corps, couleur_corps);

    SDL_Rect tete = {fx + 12, 4 + dec, 16, 14};
    Uint32 peau = SDL_MapRGB(surf->format, 230, 190, 160);
    SDL_FillRect(surf, &tete, peau);

    SDL_Rect jambe_g = {fx + 12, 38 + dec / 2, 6, 14};
    SDL_Rect jambe_d = {fx + 22, 38 - dec / 2, 6, 14};
    Uint32 pant = SDL_MapRGB(surf->format,
                             cr > 40 ? (Uint8)(cr - 30) : cr,
                             cg > 40 ? (Uint8)(cg - 30) : cg,
                             cb > 40 ? (Uint8)(cb - 30) : cb);
    SDL_FillRect(surf, &jambe_g, pant);
    SDL_FillRect(surf, &jambe_d, pant);

    if (anim_hint == J_ANIM_ATTAQUE && index_frame >= 4) {
        SDL_Rect bras = {fx + 24, 18, 16, 6};
        Uint32 epee = SDL_MapRGB(surf->format, 200, 200, 220);
        SDL_FillRect(surf, &bras, epee);
    }
}

int joueur_creer_texture(SDL_Renderer *renderer, Joueur *j)
{
    /* Joueur 1: spritesheet du lot1. */
    if (j->joueur_id == 1) {
        SDL_Surface *sheet = IMG_Load(JOUEUR_SPRITE_SHEET_PATH);
        if (sheet) {
            j->spriteset = J_SPRITESET_LOT1;
            lot1_rendre_damier_transparent(sheet);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sheet);
            SDL_FreeSurface(sheet);
            if (tex) {
                if (j->feuille_sprite)
                    SDL_DestroyTexture(j->feuille_sprite);
                j->feuille_sprite = tex;
                SDL_SetTextureBlendMode(j->feuille_sprite, SDL_BLENDMODE_BLEND);
                return 0;
            }
        }
    }

    /* Fallback (joueur 2 et/ou si le PNG n'est pas chargeable). */
    j->spriteset = J_SPRITESET_PROCEDURAL;
    const int nb_frames = 6;
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, JOUEUR_FRAME_W * nb_frames, JOUEUR_FRAME_H, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surf)
        return -1;

    for (int i = 0; i < 2; i++)
        dessiner_frame(surf, i, j->couleur_r, j->couleur_g, j->couleur_b, J_ANIM_IDLE);
    for (int i = 2; i < 4; i++)
        dessiner_frame(surf, i, j->couleur_r, j->couleur_g, j->couleur_b, J_ANIM_MARCHE);
    dessiner_frame(surf, 4, j->couleur_r, j->couleur_g, j->couleur_b, J_ANIM_SAUT);
    dessiner_frame(surf, 5, j->couleur_r, j->couleur_g, j->couleur_b, J_ANIM_ATTAQUE);

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (!tex)
        return -1;
    if (j->feuille_sprite)
        SDL_DestroyTexture(j->feuille_sprite);
    j->feuille_sprite = tex;
    SDL_SetTextureBlendMode(j->feuille_sprite, SDL_BLENDMODE_BLEND);
    return 0;
}

void joueur_initialiser(Joueur *j, SDL_Renderer *renderer, float x0, float y0,
                        int id_joueur, const JoueurConfig *cfg)
{
    memset(j, 0, sizeof(*j));
    j->x = x0;
    j->y = y0;
    j->w = JOUEUR_FRAME_W;
    j->h = JOUEUR_FRAME_H;
    j->score = 0;
    j->vies = 3;
    j->joueur_id = id_joueur;
    j->couleur_r = cfg->couleur_r;
    j->couleur_g = cfg->couleur_g;
    j->couleur_b = cfg->couleur_b;
    j->frame_courante = 0;
    j->frame_anim = 0;
    j->dernier_tick_anim = SDL_GetTicks();
    j->etat_anim = J_ANIM_IDLE;
    j->au_sol = 1;
    j->direction = 1;
    j->touche_gauche = cfg->touche_gauche;
    j->touche_droite = cfg->touche_droite;
    j->touche_saut = cfg->touche_saut;
    j->touche_course = cfg->touche_course;
    j->touche_attaque = cfg->touche_attaque;
    j->feuille_sprite = NULL;
    j->spriteset = J_SPRITESET_PROCEDURAL;
    joueur_creer_texture(renderer, j);
}

void joueur_initialiser_joueur2(Joueur *j, SDL_Renderer *renderer, float x0, float y0,
                                const JoueurConfig *cfg)
{
    joueur_initialiser(j, renderer, x0, y0, 2, cfg);
}

void joueur_liberer(Joueur *j)
{
    if (j->feuille_sprite) {
        SDL_DestroyTexture(j->feuille_sprite);
        j->feuille_sprite = NULL;
    }
}

void joueur_reinitialiser_position(Joueur *j, float x0, float y0)
{
    j->x = x0;
    j->y = y0;
    j->vx = 0;
    j->vy = 0;
}

void joueur_deplacer(Joueur *j, const Uint8 *cles, float dt_ms, int sol_y)
{
    float dt = dt_ms / 1000.0f;
    if (dt <= 0.0f)
        return;

    int gauche = cles[j->touche_gauche];
    int droite = cles[j->touche_droite];
    int course = cles[j->touche_course];
    int saut = cles[j->touche_saut];
    int attaque = cles[j->touche_attaque];
    int front_attaque = attaque && !j->touche_attaque_etait_bas;
    j->touche_attaque_etait_bas = attaque ? 1u : 0u;

    Uint32 now = SDL_GetTicks();
    if (front_attaque && j->etat_anim != J_ANIM_ATTAQUE) {
        j->fin_attaque_ms = now + DUREE_ATTAQUE_MS;
        j->etat_anim = J_ANIM_ATTAQUE;
        j->frame_anim = 0;
    }

    float vitesse = course ? VITESSE_COURSE : VITESSE_MARCHE;
    j->vx = 0.0f;
    if (gauche) {
        j->vx = -vitesse;
        j->direction = -1;
    }
    if (droite) {
        j->vx = vitesse;
        j->direction = 1;
    }

    if (saut && j->au_sol) {
        j->vy = IMPULSION_SAUT;
        j->au_sol = 0;
        if (j->etat_anim != J_ANIM_ATTAQUE || j->fin_attaque_ms <= now)
            j->etat_anim = J_ANIM_SAUT;
    }

    j->vy += GRAVITE * dt;
    j->x += j->vx * dt;
    j->y += j->vy * dt;

    int sol = sol_y - j->h;
    if (j->y >= sol) {
        j->y = (float)sol;
        j->vy = 0.0f;
        j->au_sol = 1;
    }

    if (j->x < 0.0f)
        j->x = 0.0f;
}

void joueur_animer(Joueur *j, Uint32 maintenant_ms)
{
    if (j->etat_anim == J_ANIM_MORT) {
        Uint32 intervalle = 140u;
        if (maintenant_ms - j->dernier_tick_anim >= intervalle) {
            j->dernier_tick_anim = maintenant_ms;
            j->frame_anim = (j->frame_anim + 1) % lot1_anim_nb_frames(J_ANIM_MORT);
        }
        return;
    }

    if (j->etat_anim == J_ANIM_BLESSE) {
        if (maintenant_ms >= j->fin_blesse_ms) {
            j->etat_anim = j->au_sol ? J_ANIM_IDLE : J_ANIM_SAUT;
            j->frame_anim = 0;
        } else {
            Uint32 intervalle = 90u;
            if (maintenant_ms - j->dernier_tick_anim >= intervalle) {
                j->dernier_tick_anim = maintenant_ms;
                j->frame_anim = (j->frame_anim + 1) % lot1_anim_nb_frames(J_ANIM_BLESSE);
            }
            return;
        }
    }

    if (j->etat_anim == J_ANIM_ATTAQUE) {
        if (maintenant_ms >= j->fin_attaque_ms) {
            j->etat_anim = j->au_sol ? J_ANIM_IDLE : J_ANIM_SAUT;
            j->frame_anim = 0;
        } else {
            Uint32 intervalle = 120u;
            if (maintenant_ms - j->dernier_tick_anim >= intervalle) {
                j->dernier_tick_anim = maintenant_ms;
                int max_idx = lot1_anim_nb_frames(J_ANIM_ATTAQUE) - 1;
                if (j->frame_anim < max_idx)
                    j->frame_anim++;
            }
            return;
        }
    }

    if (!j->au_sol) {
        j->etat_anim = J_ANIM_SAUT;
        j->frame_anim = 0;
        return;
    }

    if (j->vx > 1.0f || j->vx < -1.0f) {
        int course = (j->vx > VITESSE_MARCHE || j->vx < -VITESSE_MARCHE);
        j->etat_anim = course ? J_ANIM_COURSE : J_ANIM_MARCHE;
        Uint32 intervalle = course ? 70u : 110u;
        if (maintenant_ms - j->dernier_tick_anim >= intervalle) {
            j->dernier_tick_anim = maintenant_ms;
            j->frame_anim = (j->frame_anim + 1) % lot1_anim_nb_frames(j->etat_anim);
        }
    } else {
        j->etat_anim = J_ANIM_IDLE;
        if (maintenant_ms - j->dernier_tick_anim >= 400u) {
            j->dernier_tick_anim = maintenant_ms;
            j->frame_anim = (j->frame_anim + 1) % lot1_anim_nb_frames(J_ANIM_IDLE);
        }
    }
}

void joueur_afficher(SDL_Renderer *renderer, const Joueur *j)
{
    if (!j->feuille_sprite)
        return;

    SDL_Rect src;
    if (j->spriteset == J_SPRITESET_LOT1) {
        src = lot1_anim_frame_rect(j->etat_anim, j->frame_anim);
    } else {
        /* sprites procéduraux: mapping compat */
        int idx = 0;
        if (j->etat_anim == J_ANIM_MARCHE || j->etat_anim == J_ANIM_COURSE)
            idx = 2 + (j->frame_anim % 2);
        else if (j->etat_anim == J_ANIM_SAUT)
            idx = 4;
        else if (j->etat_anim == J_ANIM_ATTAQUE)
            idx = 5;
        else
            idx = (j->frame_anim % 2);
        src = (SDL_Rect){idx * JOUEUR_FRAME_W, 0, JOUEUR_FRAME_W, JOUEUR_FRAME_H};
    }

    SDL_Rect dst = {(int)j->x, (int)j->y, j->w, j->h};
    SDL_RendererFlip flip = j->direction < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, j->feuille_sprite, &src, &dst, 0.0, NULL, flip);
}

void joueur_afficher_hud(SDL_Renderer *renderer, TTF_Font *font, const Joueur *j,
                         int x_texte, int y_texte)
{
    char *buf = (char *)malloc(128);
    SDL_Surface *s = NULL;
    SDL_Texture *t = NULL;
    SDL_Color blanc = {255, 255, 255, 255};
    if (!buf)
        return;
    snprintf(buf, 128, "J%d  Score: %d   Vies: %d", j->joueur_id, j->score, j->vies);
    s = TTF_RenderUTF8_Blended(font, buf, blanc);
    if (!s)
        goto cleanup;
    t = SDL_CreateTextureFromSurface(renderer, s);
    if (!t)
        goto cleanup;
    SDL_Rect r = {x_texte, y_texte, 0, 0};
    SDL_QueryTexture(t, NULL, NULL, &r.w, &r.h);
    SDL_RenderCopy(renderer, t, NULL, &r);

cleanup:
    if (t)
        SDL_DestroyTexture(t);
    if (s)
        SDL_FreeSurface(s);
    free(buf);
}

void joueur_toucher_ennemi_placeholder(Joueur *j)
{
    if (j->vies > 0)
        j->vies--;

    Uint32 now = SDL_GetTicks();
    if (j->vies <= 0) {
        j->etat_anim = J_ANIM_MORT;
        j->frame_anim = 0;
        j->dernier_tick_anim = now;
        return;
    }
    j->etat_anim = J_ANIM_BLESSE;
    j->frame_anim = 0;
    j->dernier_tick_anim = now;
    j->fin_blesse_ms = now + DUREE_BLESSE_MS;
}

void joueur_ramasser_bonus_placeholder(Joueur *j)
{
    j->score += 10;
    if (j->vies < 5)
        j->vies++;
}
