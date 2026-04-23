#ifndef JOUEUR_H
#define JOUEUR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define JOUEUR_FRAME_W 57
#define JOUEUR_FRAME_H 95

#define JOUEUR_SPRITE_SHEET_PATH "lot1:joueur/spritesheet"

typedef enum {
    J_SPRITESET_PROCEDURAL = 0,
    J_SPRITESET_LOT1
} JoueurSpriteSet;

typedef enum {
    J_ANIM_IDLE = 0,
    J_ANIM_MARCHE,
    J_ANIM_COURSE,
    J_ANIM_SAUT,
    J_ANIM_ATTAQUE,
    J_ANIM_GRIMPE,
    J_ANIM_BLESSE,
    J_ANIM_MORT
} JoueurAnim;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int w;
    int h;
    int score;
    int vies;
    int joueur_id;
    Uint8 couleur_r;
    Uint8 couleur_g;
    Uint8 couleur_b;
    SDL_Texture *feuille_sprite;
    JoueurSpriteSet spriteset;
    int frame_courante;
    int frame_anim; /* index de frame dans l'animation en cours */
    Uint32 dernier_tick_anim;
    JoueurAnim etat_anim;
    Uint32 fin_attaque_ms;
    Uint32 fin_blesse_ms;
    int au_sol;
    int direction;
    SDL_Scancode touche_gauche;
    SDL_Scancode touche_droite;
    SDL_Scancode touche_saut;
    SDL_Scancode touche_course;
    SDL_Scancode touche_attaque;
    Uint8 touche_attaque_etait_bas;
} Joueur;

typedef struct {
    Uint8 couleur_r;
    Uint8 couleur_g;
    Uint8 couleur_b;
    SDL_Scancode touche_gauche;
    SDL_Scancode touche_droite;
    SDL_Scancode touche_saut;
    SDL_Scancode touche_course;
    SDL_Scancode touche_attaque;
} JoueurConfig;

int joueur_creer_texture(SDL_Renderer *renderer, Joueur *j);

void joueur_initialiser(Joueur *j, SDL_Renderer *renderer, float x0, float y0,
                        int id_joueur, const JoueurConfig *cfg);

void joueur_initialiser_joueur2(Joueur *j, SDL_Renderer *renderer, float x0, float y0,
                                const JoueurConfig *cfg);

void joueur_liberer(Joueur *j);

void joueur_reinitialiser_position(Joueur *j, float x0, float y0);

void joueur_deplacer(Joueur *j, const Uint8 *cles, float dt_ms, int sol_y);

void joueur_animer(Joueur *j, Uint32 maintenant_ms);

void joueur_afficher(SDL_Renderer *renderer, const Joueur *j);

void joueur_afficher_hud(SDL_Renderer *renderer, TTF_Font *font, const Joueur *j,
                         int x_texte, int y_texte);

void joueur_toucher_ennemi_placeholder(Joueur *j);

void joueur_ramasser_bonus_placeholder(Joueur *j);

#endif
