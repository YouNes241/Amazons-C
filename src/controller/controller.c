#include "../../include/controller.h"
#include "../../include/ai_factory.h"
#include "../../include/ai_interface.h"
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

/* ---Static variables (Global game context) --- */

static GameState *g_current_game = NULL;
static game_config_t *g_config = NULL;
static bool *g_game_running = NULL;
static NetworkManager *g_net_manager = NULL;
static PlayerColor g_my_role = PLAYER_WHITE; // Par défaut pour le serveur
static AI_Interface g_ai;
static bool g_ai_initialized = false;
static bool g_in_network_match = false;

/* --- Time management --- */

static time_t g_turn_start_time;
static int g_timeout_limit = 60;
static bool g_timeout_sent = false;

static char *commands_list[]
    = { "new",    "help",    "quit",         "load", "save",
        "pause",  "hint",    "undo",         "redo", "show",
        "set",    "details", "start server", "join", "players",
        "invite", "accept",  "decline",      NULL };

/* ==========================================================================
   HELPERS UI & PROMPT
   ==========================================================================
 */

static bool
is_it_ai_turn (game_config_t *config, PlayerColor current_p)
{
  if (current_p == PLAYER_WHITE)
    return config->ai_white;
  if (current_p == PLAYER_BLACK)
    return config->ai_black;
  return false;
}

/**
 * @brief Constructs the prompt string and applies it to Readline (without
 * displaying it).
 */
static void
update_prompt_string ()
{
  static char prompt_str[PROMPT_MAX_LEN];
  const char *turn_str
      = (g_current_game->current_player == PLAYER_WHITE) ? "White" : "Black";

  // If we are on a network
  if (g_net_manager != NULL && g_net_manager->current_mode != NET_MODE_NONE)
    {
      if (g_in_network_match)
        {
          // In a game: Show Role and Turn
          const char *my_color_str = (g_my_role == PLAYER_WHITE) ? "W" : "B";
          snprintf (prompt_str, sizeof (prompt_str), "[%s] Turn: %s >> ",
                    my_color_str, turn_str);
        }
      else
        {
          // Not in a game yet: Show LOBBY prompt
          snprintf (prompt_str, sizeof (prompt_str), "[LOBBY] >> ");
        }
    }
  else
    {
      // In local mode, we just display whose turn it is.
      snprintf (prompt_str, sizeof (prompt_str), "[%s] >> ", turn_str);
    }

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
  rl_replace_line ("", READLINE_NO_CLEAR); // Clears the current line's memory
  rl_on_new_line ();                       // Prepares ReadLine for a new line
  rl_redisplay ();                         // Forces the display
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
   network callback
   ==========================================================================
 */

static void
on_game_started (PlayerColor color)
{
  g_my_role = color;
  g_in_network_match = true;
  g_turn_start_time = time (NULL); // The timer starts
  printf ("\n[GAME] The game begins! You play the %s.\n",
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

      // Reset the timer because it's our turn now

      g_timeout_sent = false;
      g_turn_start_time = time (NULL);

      cli_display_board (g_current_game);
      refresh_prompt ();
    }
}

/**
 * @brief Called when the network informs us that the opponent has timed out
 */
static void
on_network_timeout_received ()
{
  printf ("\n[NETWORK] The opponent has exceeded their time limit! It's your "
          "turn to play!.\n");

  // 1. We are changing the current player locally
  g_current_game->current_player
      = (g_current_game->current_player == PLAYER_WHITE) ? PLAYER_BLACK
                                                         : PLAYER_WHITE;

  // 2. MOST IMPORTANTLY: We reset our timer to begin our own 60 seconds !

  g_timeout_sent = false;
  g_turn_start_time = time (NULL);
  // 3. We refresh the display
  cli_display_board (g_current_game);
  refresh_prompt ();
}

static void
on_player_joined (const char *name)
{
  (void)name;
  refresh_prompt ();
}

static void
on_player_disconnected (const char *reason)
{
  (void)reason;
  if (g_in_network_match)
    {
      printf ("\n[LOBBY] Your opponent disconnected. Returning to LOBBY.\n");
    }
  g_in_network_match = false; // return to LOBBY
  refresh_prompt ();
}

static void
on_game_error (const char *err)
{
  (void)err;
  refresh_prompt ();
}

static void
on_invitation_received (int challenger_id)
{
  printf ("\n[LOBBY] Player %d invited you to a game!\n", challenger_id);
  printf ("Type 'accept %d' to play, or 'decline %d' to refuse.\n",
          challenger_id, challenger_id);
  refresh_prompt ();
}

static void
on_list_received (const char *list_text)
{
  printf ("\n=== LOBBY ===\n%s\n===============\n", list_text);
  refresh_prompt ();
}

/* ==========================================================================
   COMMAND LOGIC (handle_command)
   ==========================================================================
 */

void
handle_command (char *input_buffer, game_config_t *config, bool *game_running,
                GameState *current_game, NetworkManager *net_manager)
{

  // 1. QUIT
  if (strcmp (input_buffer, "quit") == 0)
    {
      *game_running = false;
      return;
    }

  // 2. HELP & CONFIGURATION
  else if (strncmp (input_buffer, "help", CMD_LEN_HELP) == 0)
    {
      printf ("\n=== COMMAND LIST ===\n");
      printf ("--- Game Management ---\n");
      printf ("  new         : Start a new local game\n");
      printf ("  quit        : Exit the application\n");
      printf ("  show        : Display the board\n");
      printf ("  undo/redo   : Cancel or replay a move\n");
      printf ("  save/load   : Save or load a game state\n");
      printf ("  hint        : Get AI assistance\n");
      printf ("  set/details : Configure game options\n");

      printf ("\n--- Network & LOBBY ---\n");
      printf ("  server start [port]        : Host a new server\n");
      printf ("  join [IP:Port] [username]  : Connect to a server\n");
      printf (
          "  players                    : List all players in the LOBBY\n");
      printf ("  invite <ID>                : Invite a player to a game\n");
      printf ("  accept <ID>                : Accept an invitation\n");
      printf ("  decline <ID>               : Decline an invitation\n");

      printf ("\n--- Playing a Move ---\n");
      printf (
          "  Format: [QueenStart]-[QueenEnd]([ArrowEnd])  Ex: d10-d9(d8)\n");
      printf ("====================\n\n");
      return;
    }
  else if (strncmp (input_buffer, "details", CMD_LEN_DETAILS) == 0)
    {
      display_config (config);
      return;
    }

  // 3. NETWORK: SERVER START
  else if (strncmp (input_buffer, "server start", CMD_LEN_SERVER_START) == 0)
    {
      int port = DEFAULT_PORT;
      if (strlen (input_buffer) > CMD_LEN_SERVER_START + CMD_SPACE_OFFSET)
        sscanf (input_buffer + CMD_LEN_SERVER_START + CMD_SPACE_OFFSET, "%d",
                &port);
      NetworkManager_ServerStart (net_manager, port);
      return;
    }

  // 4. NETWORK: JOIN (Full parsing)
  else if (strncmp (input_buffer, "join", CMD_LEN_JOIN) == 0)
    {
      char ip[MAX_IP_LEN + 1] = DEFAULT_IP;
      int port = DEFAULT_PORT;
      char username[MAX_USERNAME_LEN] = "DEFAULT_USERNAME";

      if (strlen (input_buffer) > CMD_LEN_JOIN + CMD_SPACE_OFFSET)
        {
          char arg1[MAX_IP_LEN + 1], arg2[MAX_USERNAME_LEN + 1];
          int parsed = sscanf (input_buffer + CMD_LEN_JOIN + CMD_SPACE_OFFSET,
                               "%63s %31s", arg1, arg2);
          if (parsed >= 1)
            {
              char *colon = strchr (arg1, ':');
              if (colon)
                {
                  *colon = '\0';
                  strncpy (ip, arg1, MAX_IP_LEN);
                  port = atoi (colon + 1);
                }
              else
                {
                  strncpy (ip, arg1, MAX_IP_LEN);
                }
            }
          if (parsed == 2)
            strncpy (username, arg2, MAX_USERNAME_LEN);
        }

      if (NetworkManager_ClientJoin (net_manager, ip, port) == 0)
        {
          NetworkManager_SendJoin (net_manager, username);
        }
      return;
    }

  else if (strcmp (input_buffer, "players") == 0)
    {
      if (net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (net_manager);
        }
      else if (net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (net_manager, list_buffer,
                                              sizeof (list_buffer));
          printf ("\n=== SERVER LOBBY ===\n%s\n====================\n",
                  list_buffer);
        }
      else
        {
          printf ("You must start or join a server first to see players.\n");
        }
      return;
    }
  else if (strncmp (input_buffer, "invite ", CMD_LEN_INVITE) == 0)
    {
      if (net_manager->current_mode != NET_MODE_CLIENT)
        {
          printf ("You must join a server first to invite someone.\n");
          return;
        }
      int target_id;
      if (sscanf (input_buffer + CMD_LEN_INVITE, "%d", &target_id) == 1)
        {
          if (NetworkManager_SendInvite (net_manager, target_id) == 0)
            printf ("Invitation sent to player %d. Waiting for response...\n",
                    target_id);
        }
      return;
    }
  else if (strncmp (input_buffer, "accept ", CMD_LEN_ACCEPT) == 0)
    {
      if (net_manager->current_mode != NET_MODE_CLIENT)
        return;
      int challenger_id;
      if (sscanf (input_buffer + CMD_LEN_ACCEPT, "%d", &challenger_id) == 1)
        {
          if (NetworkManager_SendAccept (net_manager, challenger_id) == 0)
            printf ("Accepted invitation from player %d.\n", challenger_id);
        }
      return;
    }
  else if (strncmp (input_buffer, "decline ", CMD_LEN_DECLINE) == 0)
    {
      if (net_manager->current_mode != NET_MODE_CLIENT)
        return;
      int challenger_id;
      if (sscanf (input_buffer + CMD_LEN_DECLINE, "%d", &challenger_id) == 1)
        {
          if (NetworkManager_SendDecline (net_manager, challenger_id) == 0)
            printf ("Declined invitation from player %d.\n", challenger_id);
        }
      return;
    }

  // 5. GAME MANAGEMENT (UNDO / REDO / SAVE / LOAD / NEW)
  else if (strncmp (input_buffer, "new", CMD_LEN_NEW) == 0)
    {
      printf ("New local game.\n");
      game_init (current_game);
      g_turn_start_time = time (NULL);
      return;
    }
  else if (strncmp (input_buffer, "undo", CMD_LEN_UNDO) == 0)
    {
      bool vs_ai = (config->ai_white || config->ai_black);
      bool is_local = (net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (undo_move (current_game))
            {
              undo_move (current_game);
              printf ("Move(s) undone successfully.\n");
              cli_display_board (current_game);
            }
          else
            {
              printf ("Impossible to undo.\n");
            }
        }
      else
        {
          printf ("The 'undo' command is only available in solo mode against "
                  "the AI.\n");
        }
      return;
    }
  else if (strncmp (input_buffer, "redo", CMD_LEN_REDO) == 0)
    {
      bool vs_ai = (config->ai_white || config->ai_black);
      bool is_local = (g_net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (redo_move (current_game))
            {
              redo_move (current_game);

              printf ("Move(s) redone successfully.\n");
              cli_display_board (current_game);
            }
          else
            {
              printf ("Impossible to redo : no move to redo.\n");
            }
        }
      else
        {
          printf ("The 'redo' command is only available in solo mode against "
                  "the AI.\n");
        }
      return;
    }
  else if (strncmp (input_buffer, "save ", CMD_LEN_SAVE) == 0)
    {
      printf ("Saved in: %s\n", &input_buffer[CMD_LEN_SAVE]);
      return;
    }
  else if (strncmp (input_buffer, "load ", CMD_LEN_LOAD) == 0)
    {
      printf ("Loading of : %s\n", &input_buffer[CMD_LEN_LOAD]);
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
  else if (strncmp (input_buffer, "show", CMD_LEN_SHOW) == 0)
    {
      cli_display_board (current_game);
      return;
    }

  // 6. GAME LOGIC (MOVES)
  Move player_move;
  if (parse_coordinates (input_buffer, &player_move) == PARSE_MOVE_SUCCESS)
    {
      // --- Turn lock (Active ONLY in network mode) ---
      if (net_manager->current_mode != NET_MODE_NONE)
        {
          if (!g_in_network_match)
            {
              printf ("[ERROR] You are in the LOBBY! Use 'invite' to start a "
                      "game.\n");
              return;
            }

          if (current_game->current_player != g_my_role)
            {
              printf ("[ERROR] It's not your turn! Wait for your opponent.\n");
              return;
            }
        }

      if (is_move_legal (current_game, player_move))
        {
          apply_move (current_game, player_move);
          NetworkManager_SendMove (
              net_manager, player_move); // TODO condition si on est en réseaux
          g_timeout_sent = false;
          // We played, we reset the timer for the wait.
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

  printf ("\n"); // Visually validate the entry

  if (line[0] != '\0')
    {
      add_history (line);
      rl_replace_line ("", READLINE_NO_CLEAR);
      handle_command (line, g_config, g_game_running, g_current_game,
                      g_net_manager);
    }
  else
    {
      rl_replace_line ("", READLINE_NO_CLEAR);
    }
  free (line);

  if (*g_game_running)
    {
      // 1. The plateau is displayed at the end of the processing.
      cli_display_board (g_current_game);

      // 2. We are updating the text of the prompt
      update_prompt_string ();

      // 3. The line break is signaled to Readline.
      // The redisplay will be handled automatically by Readline when you exit
      // this page.
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
  net_events.OnPlayerJoined = on_player_joined;
  net_events.OnGameError = on_game_error;
  net_events.OnInvitationReceived = on_invitation_received;
  net_events.OnPlayerDisconnected = on_player_disconnected;
  net_events.OnTimeoutReceived = on_network_timeout_received;
  net_events.OnListReceived = on_list_received;
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

  // Installing the Readline handler
  rl_callback_handler_install ("[White] >> ", linehandler);

  struct pollfd fds[STDIN_FD_COUNT];
  fds[STDIN_FD_INDEX].fd = STDIN_FILENO;
  fds[STDIN_FD_INDEX].events = POLLIN;

  if ((config->ai_white || config->ai_black))
    {
      ConfigMap empty_params = { .list = NULL, .count = 0 };
      g_ai = create_ai ("random", empty_params);
      g_ai_initialized = (g_ai.vtable != NULL);
    }

  while (game_running)
    {
      if (is_game_over (g_current_game, g_current_game->current_player))
        {
          cli_display_board (g_current_game);
          PlayerColor winner = get_winner (g_current_game);
          printf ("\n GAME OVER ! \n");
          printf ("Player %s is blocked.\n",
                  (g_current_game->current_player == PLAYER_WHITE ? "White"
                                                                  : "Black"));
          printf ("Victory of the %s !\n",
                  (winner == PLAYER_WHITE ? "Whites" : "Blacks"));

          game_running = false;
          break;
        }
      NetworkManager_Tick (&net_manager);

      if (g_ai_initialized
          && is_it_ai_turn (config, g_current_game->current_player))
        {
          printf ("\n[AI] Thinking...\n");
          Move ai_move = g_ai.vtable->get_best_move (&g_ai, *g_current_game);

          if (is_move_legal (g_current_game, ai_move))
            {
              apply_move (g_current_game, ai_move);
              g_turn_start_time = time (NULL);
              cli_display_board (g_current_game);
              refresh_prompt ();
            }
          else
            {
              printf (
                  "\n[ERROR] AI suggested an illegal move or is blocked.\n");
              printf ("Game ended.\n");
              game_running = false;
            }
        }

      // --- SYNCHRONIZED TIMEOUT LOGIC ---
      if (g_in_network_match && g_current_game->current_player == g_my_role)
        {

          time_t now = time (NULL);
          if (difftime (now, g_turn_start_time) >= g_timeout_limit
              && !g_timeout_sent)
            {
              printf ("\n[TIMEOUT] Time's up! Passing the turn to your "
                      "adversary..\n");

              // Turn switch
              g_current_game->current_player
                  = (g_current_game->current_player == PLAYER_WHITE)
                        ? PLAYER_BLACK
                        : PLAYER_WHITE;

              // Network send
              NetworkManager_SendTimeout (&net_manager);

              g_timeout_sent = true;
              g_turn_start_time = now;

              cli_display_board (g_current_game);
              refresh_prompt ();
            }
        }
      else if (g_in_network_match) // On ne reset le temps que si on joue
        {
          g_timeout_sent = false;
          g_turn_start_time = time (NULL);
        }

      if (poll (fds, STDIN_FD_COUNT, POLL_STDIN_TIMEOUT_MS) > 0
          && (fds[STDIN_FD_INDEX].revents & POLLIN))
        rl_callback_read_char ();
    }

  rl_callback_handler_remove ();
  if (g_ai_initialized && g_ai.vtable->clean)
    {
      g_ai.vtable->clean (&g_ai);
    }
  NetworkManager_Cleanup (&net_manager);
}