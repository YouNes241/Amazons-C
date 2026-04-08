#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MIN_BOARD_SIZE 4
#define MAX_BOARD_SIZE 11
#define MAX_POSSIBLE_MOVES 4000
#define MAX_BEAM_LIMIT 100
#define MAX_BOARD_SQUARES (MAX_BOARD_SIZE * MAX_BOARD_SIZE)
#define DEFAULT_BOARD_SIZE 10
#define DEFAULT_TIME_BLITZ 1800 // 30 minutes default
#define DEFAULT_AI_COLOR 1      // AI plays black by default
#define VERSION "0.1.0"
#define MAX_AI_PARAMS 10
#define MAX_AI_STR_LEN 32

/*
 * The game can only be in
 * one of these modes at a time.
 */
typedef enum
{
  MODE_INTERACTIVE, // Default console mode
  MODE_CONTEST,     // Tournament mode without interface
  MODE_GUI          // Graphical interface mode
} game_mode_t;

typedef enum
{
  LANG_FR,
  LANG_EN
} lang_t;

typedef struct
{
  bool enabled;
  char name[MAX_AI_STR_LEN]; // ex: "minimax", "mcts"
  char param_keys[MAX_AI_PARAMS][MAX_AI_STR_LEN];
  char param_vals[MAX_AI_PARAMS][MAX_AI_STR_LEN];
  int param_count;
} ai_config_t;
typedef struct
{
  bool blitz_mode;
  bool verbose;
  bool start_server;
  bool daemon_mode;
  int board_size;
  int time_per_player;
  int server_port;
  game_mode_t mode;
  lang_t language;

  ai_config_t ai_white_cfg;
  ai_config_t ai_black_cfg;
} game_config_t;

void init_default_config (game_config_t *config);
void display_config (game_config_t *config);
void load_config_file (game_config_t *config);
void get_config_path (char *path, size_t size);
#endif
