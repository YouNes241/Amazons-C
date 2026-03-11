#include "../../include/controller.h"
#include "../../include/cli_view.h"
#include "../../include/game_engine.h"
#include "../../include/input.h"
#include "../../include/network_common.h"
#include <poll.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* --- Variables statiques (Contexte global du jeu) --- */
static GameState *g_current_game = NULL;
static game_config_t *g_config = NULL;
static bool *g_game_running = NULL;
static NetworkManager *g_net_manager = NULL;
static PlayerColor g_my_role = PLAYER_WHITE; // Par défaut pour le serveur

/* --- Gestion du temps --- */
static time_t g_turn_start_time;
static int g_timeout_limit = 60;
static bool g_timeout_sent = false;

static char *commands_list[]
    = { "new",  "help", "quit", "load", "save",    "pause", "hint",
        "undo", "redo", "show", "set",  "details", NULL };

/* ==========================================================================
   HELPERS UI & PROMPT
   ==========================================================================
 */

/**
 * @brief Constructs the prompt string and applies it to Readline (without
 * displaying it).
 */
static void
update_prompt_string ()
{
  static char prompt_str[64];
  const char *my_color_str = (g_my_role == PLAYER_WHITE) ? "W" : "B";
  const char *turn_str
      = (g_current_game->current_player == PLAYER_WHITE) ? "White" : "Black";

  sprintf (prompt_str, "[%s] Turn: %s >> ", my_color_str, turn_str);
  rl_set_prompt (prompt_str);
}

/**
 * @brief Forces immediate prompt refresh (used for network events)
 * network
 */
static void
refresh_prompt ()
{
  update_prompt_string ();
  rl_replace_line ("", 0); // Clears the current line's memory
  rl_on_new_line ();       // Prepares ReadLine for a new line
  rl_redisplay ();         // Forces the display
}

/* --- Autocomplete --- */
char *
command_generator (const char *text, int state)
{
  static int list_index, len;
  char *name;
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }
  while ((name = commands_list[list_index++]))
    {
      if (strncmp (name, text, len) == 0)
        return strdup (name);
    }
  return NULL;
}

char **
amazons_completion (const char *text, int start, int end)
{
  (void)end;
  rl_attempted_completion_over = 1;
  if (start == 0)
    return rl_completion_matches (text, command_generator);
  return NULL;
}

/* ==========================================================================
   CALLBACKS RÉSEAU
   ==========================================================================
 */

static void
on_game_started (PlayerColor color)
{
  g_my_role = color;
  g_turn_start_time = time (NULL); // The timer starts
  printf ("\n[[GAME] The game begins! You play the %s.\n",
          (color == PLAYER_WHITE) ? "White" : "Black");
  refresh_prompt ();
}

static void
on_network_move_received (Move m)
{
  if (is_move_legal (g_current_game, m))
    {
      printf ("\n");
      apply_move (g_current_game, m);
      printf ("[NETWORK] The opponent played !\n");

      // Reset du timer car c'est maintenant à notre tour

      g_timeout_sent = false;
      g_turn_start_time = time (NULL);

      cli_display_board (g_current_game);
      refresh_prompt ();
    }
}

/**
 * @brief Appelé quand le réseau nous informe que l'adversaire a timeout
 */
static void
on_network_timeout_received ()
{
  printf ("\n[NETWORK] The opponent has exceeded their time limit! It's your "
          "turn to play!.\n");

  // 1. On change le joueur actuel localement
  g_current_game->current_player
      = (g_current_game->current_player == PLAYER_WHITE) ? PLAYER_BLACK
                                                         : PLAYER_WHITE;

  // 2. SURTOUT : On reset notre chrono pour commencer nos 60s à nous !

  g_timeout_sent = false;
  g_turn_start_time = time (NULL);
  // 3. On rafraîchit l'affichage
  cli_display_board (g_current_game);
  refresh_prompt ();
}

/* ==========================================================================
   LOGIQUE DES COMMANDES (handle_command)
   ==========================================================================
 */

void
handle_command (char *input_buffer, game_config_t *config, bool *game_running,
                GameState *current_game, NetworkManager *net_manager)
{

  // 1. QUITTER
  if (strcmp (input_buffer, "quit") == 0)
    {
      *game_running = false;
      return;
    }

  // 2. AIDE & CONFIG
  else if (strncmp (input_buffer, "help", 4) == 0)
    {
      printf ("Aide : new, help, quit, load, save, pause, hint, undo, redo, "
              "show, set, details\n");
      return;
    }
  else if (strncmp (input_buffer, "details", 7) == 0)
    {
      display_config (config);
      return;
    }

  // 3. RÉSEAU : SERVER START
  else if (strncmp (input_buffer, "server start", 12) == 0)
    {
      int port = 12345;
      if (strlen (input_buffer) > 13)
        sscanf (input_buffer + 13, "%d", &port);
      NetworkManager_ServerStart (net_manager, port);
      return;
    }

  // 4. RÉSEAU : JOIN (Parsing complet)
  else if (strncmp (input_buffer, "join", 4) == 0)
    {
      char ip[64] = "127.0.0.1";
      int port = 12345;
      char username[32] = "Anonymous";

      if (strlen (input_buffer) > 5)
        {
          char arg1[64], arg2[32];
          int parsed = sscanf (input_buffer + 5, "%63s %31s", arg1, arg2);
          if (parsed >= 1)
            {
              char *colon = strchr (arg1, ':');
              if (colon)
                {
                  *colon = '\0';
                  strncpy (ip, arg1, 63);
                  port = atoi (colon + 1);
                }
              else
                {
                  strncpy (ip, arg1, 63);
                }
            }
          if (parsed == 2)
            strncpy (username, arg2, 31);
        }

      if (NetworkManager_ClientJoin (net_manager, ip, port) == 0)
        {
          NetworkManager_SendJoin (net_manager, username);
        }
      return;
    }

  // 5. GESTION DE PARTIE (UNDO / REDO / SAVE / LOAD / NEW)
  else if (strncmp (input_buffer, "new", 3) == 0)
    {
      printf ("New local game.\n");
      game_init (current_game);
      g_turn_start_time = time (NULL);
      return;
    }
  else if (strncmp (input_buffer, "undo", 4) == 0)
    {
      printf ("Cancellation: (logic to be implemented)\n");
      return;
    }
  else if (strncmp (input_buffer, "redo", 4) == 0)
    {
      printf ("Action replayed: (logic to be implemented)\n");
      return;
    }
  else if (strncmp (input_buffer, "save ", 5) == 0)
    {
      printf ("Saved in: %s\n", &input_buffer[5]);
      return;
    }
  else if (strncmp (input_buffer, "load ", 5) == 0)
    {
      printf ("Loading of : %s\n", &input_buffer[5]);
      return;
    }
  else if (strcmp (input_buffer, "pause") == 0)
    {
      printf ("Game paused...\n");
      return;
    }
  else if (strcmp (input_buffer, "hint") == 0)
    {
      printf ("Tip: (AI in progress)...)\n");
      return;
    }
  else if (strncmp (input_buffer, "show", 4) == 0)
    {
      cli_display_board (current_game);
      return;
    }

  // 6. LOGIQUE DE JEU (COUPS)
  Move player_move;
  if (parse_coordinates (input_buffer, &player_move) == 0)
    {
      // VERROU DE TOUR
      if (current_game->current_player != g_my_role)
        {
          printf ("[ERROR] It's not your turn! Wait for your opponent.\n");
          return;
        }

      if (is_move_legal (current_game, player_move))
        {
          apply_move (current_game, player_move);
          NetworkManager_SendMove (net_manager, player_move);
          g_timeout_sent = false;
          // On a joué, on reset le timer pour l'attente
          g_turn_start_time = time (NULL);

          if (is_game_over (current_game, current_game->current_player))
            {
              cli_display_board (current_game);
              PlayerColor winner = get_winner (current_game);
              printf ("Player %s won !\n",
                      (winner == PLAYER_WHITE ? "White" : "Black"));
              *game_running = false;
            }
        }
      else
        {
          printf ("An illegal blow according to the rules.\n");
        }
    }
  else
    {
      printf ("Invalid format. Use: a1-a3(a4) format or type 'help'.\n");
    }
}

/* ==========================================================================
   LINEHANDLER (Readline)
   ==========================================================================
 */

static void
linehandler (char *line)
{
  if (line == NULL)
    return;

  printf ("\n"); // Valide visuellement l'entrée

  if (line[0] != '\0')
    {
      add_history (line);
      rl_replace_line ("", 0);
      handle_command (line, g_config, g_game_running, g_current_game,
                      g_net_manager);
    }
  else
    {
      rl_replace_line ("", 0);
    }
  free (line);

  if (*g_game_running)
    {
      // 1. On affiche le plateau à la fin du traitement
      cli_display_board (g_current_game);

      // 2. On met à jour le texte du prompt
      update_prompt_string ();

      // 3. On signale le saut de ligne à Readline.
      // Le redisplay sera fait automatiquement par Readline en sortant d'ici.
      rl_on_new_line ();
    }
}
/* ==========================================================================
   RUN_INTERACTIVE_MODE
   ==========================================================================
 */

void
run_interactive_mode (game_config_t *config)
{
  GameState current_game;
  current_game.size = config->board_size;
  game_init (&current_game);

  NetworkManager net_manager;
  NetworkEvents net_events = { 0 };
  net_events.OnMoveReceived = on_network_move_received;
  net_events.OnGameStarted = on_game_started;
  net_events.OnPlayerJoined = NULL;
  net_events.OnPlayerDisconnected = (void (*) (const char *))refresh_prompt;
  net_events.OnTimeoutReceived = on_network_timeout_received;
  NetworkManager_Init (&net_manager, net_events);

  bool game_running = true;
  g_current_game = &current_game;
  g_config = config;
  g_game_running = &game_running;
  g_net_manager = &net_manager;
  g_turn_start_time = time (NULL);

  rl_attempted_completion_function = amazons_completion;
  cli_clear_screen ();
  printf ("Welcome to the Amazones' Game !\n");
  cli_display_board (&current_game);

  // Installation du handler Readline
  rl_callback_handler_install ("[White] >> ", linehandler);

  struct pollfd fds[1];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  while (game_running)
    {
      NetworkManager_Tick (&net_manager);

      // --- LOGIQUE DE TIMEOUT SYNCHRONISÉE ---
      if (g_current_game->current_player == g_my_role)
        {
          if (NetworkManager_HasOpponent (&net_manager))
            {
              time_t now = time (NULL);
              if (difftime (now, g_turn_start_time) >= g_timeout_limit
                  && !g_timeout_sent)
                {
                  printf ("\n[TIMEOUT] Time's up! Passing the turn to your "
                          "adversary..\n");

                  // Switch de tour
                  g_current_game->current_player
                      = (g_current_game->current_player == PLAYER_WHITE)
                            ? PLAYER_BLACK
                            : PLAYER_WHITE;

                  // Envoi réseau
                  NetworkManager_SendTimeout (&net_manager);

                  g_timeout_sent = true;
                  g_turn_start_time = now;

                  cli_display_board (g_current_game);
                  refresh_prompt ();
                }
            }
        }
      else
        {
          // SI CE N'EST PAS MON TOUR :
          // On reset g_timeout_sent pour être prêt pour notre prochain tour
          g_timeout_sent = false;
          // On garde turn_start_time à jour pour avoir nos 60s complètes quand
          // le tour arrive
          g_turn_start_time = time (NULL);
        }

      if (poll (fds, 1, 100) > 0 && (fds[0].revents & POLLIN))
        rl_callback_read_char ();
    }

  rl_callback_handler_remove ();
  NetworkManager_Cleanup (&net_manager);
}