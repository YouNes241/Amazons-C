#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h> 

/* 
 * Le jeu ne peut être que dans 
 * un seul de ces modes à la fois.
 */
typedef enum
{
  MODE_INTERACTIVE, /* Mode console par défaut */
  MODE_CONTEST,     /* Mode tournoi sans interface  */
  MODE_GUI          /* Mode interface graphique  */
} game_mode_t;

typedef enum
{
  LANG_FR,
  LANG_EN
} lang_t;


typedef struct
{
  int board_size;           /* Taille du plateau */
  game_mode_t mode;         /* Le mode de jeu actuel choisi */
  bool blitz_mode;          /* Vrai si le mode blitz est activé  */
  int time_per_player;
  bool ai_enabled;          /* Vrai si on joue contre l'ia */
  bool verbose;            /* Pour le debug */
  lang_t language;  
} game_config_t;


#define MIN_BOARD_SIZE 4
#define MAX_BOARD_SIZE 11
#define DEFAULT_BOARD_SIZE 10
#define DEFAULT_TIME_BLITZ 1800 /* 30 minutes par défaut */
#define DEFAULT_AI_COLOR 1 /* L'IA joue en noir par défaut */

#endif /* CONFIG_H */