/**
 * @file cli_commands.c
 * @brief Implementation of CLI command handling for the Amazons game.
 */

#include "cli_commands.h"
#include "ai_factory.h"
#include "cli_view.h"
#include "controller.h"
#include "game_engine.h"
#include "i18n.h"
#include "input.h"
#include "save.h"
#include "utils.h"
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CliContext *g_ctx_ref = NULL;

/** @brief Clears the global CLI context.
 * @details This function sets the global CLI context reference to NULL.
 */
void
clear_global_cli_context (void)
{
  g_ctx_ref = NULL;
}
// --- Autocomplete ---
static char *commands_list[] = { "new",
                                 "help",
                                 "quit",
                                 "load",
                                 "save",
                                 "pause",
                                 "hint",
                                 "undo",
                                 "redo",
                                 "show board",
                                 "show history",
                                 "show time",
                                 "show configuration",
                                 "set",
                                 "start server",
                                 "server status",
                                 "server stop",
                                 "join",
                                 "players",
                                 "accept",
                                 "decline",
                                 "away",
                                 "back",
                                 "cancel",
                                 "ping",
                                 "scoreboard",
                                 NULL };

/** @brief Generates command completions for Readline.
 * @details This function is used by Readline to provide command completions.
 */
static char *
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

/** @brief Generates command completions for Readline.
 * @details This function is used by Readline to provide command completions.
 */
char **
amazons_completion (const char *text, int start, int end)
{
  (void)end;
  rl_attempted_completion_over = 1;
  if (start == 0)
    return rl_completion_matches (text, command_generator);
  return NULL;
}

/** @name UI Helpers and Prompt Management
 */
///@{

/** @brief Updates the prompt string based on the current context.
 * @details This function updates the prompt string displayed to the user based
 * on the current game state and context.
 */
void
update_prompt_string (CliContext *ctx)
{
  static char prompt_str[PROMPT_MAX_LEN];
  const char *turn_str = (ctx->current_game->current_player == PLAYER_WHITE)
                             ? _ ("White")
                             : _ ("Black");

  if (ctx->in_main_menu)
    {
      snprintf (prompt_str, sizeof (prompt_str), "[MENU] >> ");
      rl_set_prompt (prompt_str);
      return;
    }
  else if (ctx->net_manager != NULL
           && ctx->net_manager->current_mode != NET_MODE_NONE)
    {
      if (ctx->in_network_match)
        {
          const char *my_color_str
              = (ctx->my_role == PLAYER_WHITE) ? _ ("W") : _ ("B");
          snprintf (prompt_str, sizeof (prompt_str), _ ("[%s] Turn: %s >> "),
                    my_color_str, turn_str);
        }
      else
        {
          if (ctx->is_away)
            {
              snprintf (prompt_str, sizeof (prompt_str), "%s",
                        _ ("[AWAY] >> "));
            }
          else
            {
              snprintf (prompt_str, sizeof (prompt_str), "%s",
                        _ ("[LOBBY] >> "));
            }
        }
    }
  else
    {
      snprintf (prompt_str, sizeof (prompt_str), _ ("[%s] >> "), turn_str);
    }
  rl_set_prompt (prompt_str);
}

/** @brief Refreshes the CLI prompt.
 * @details This function updates the CLI prompt based on the current context.
 */
void
refresh_prompt (CliContext *ctx)
{
  update_prompt_string (ctx);
  rl_replace_line ("", READLINE_NO_CLEAR);
  rl_on_new_line ();
  rl_redisplay ();
}

/** @brief Executes the hint command.
 * @details This function provides advice to the player based on the current
 * game state.
 */
void
execute_hint (CliContext *ctx)
{
  if (!ctx->current_game)
    {
      printf ("%s", "[ERROR] No game in progress to get advice.\n");
      return;
    }

  printf ("%s", "[AI] Initializing advice engine...\n");
  ConfigMap empty_params = { 0, NULL };

  AI_Interface hint_ai = create_ai ("minimax", empty_params);

  if (hint_ai.vtable == NULL)
    {
      printf ("[ERROR] %s\n", "Failed to load the advice engine.");
      return;
    }

  printf ("%s", "[AI] Analyzing the situation...\n");

  Move sug = hint_ai.vtable->get_best_move (&hint_ai, *(ctx->current_game));

  char src[POS_STRING_MAX_LEN], dst[POS_STRING_MAX_LEN],
      arr[POS_STRING_MAX_LEN];
  pos_to_string (sug.queen_src, src, sizeof (src));
  pos_to_string (sug.queen_dst, dst, sizeof (dst));
  pos_to_string (sug.arrow_dst, arr, sizeof (arr));

  printf ("\n[ADVICE] Based on the current state:\n");
  printf (" - Move the queen from %s to %s\n", src, dst);
  printf (" - Shoot the arrow at %s\n", arr);

  if (hint_ai.vtable->clean)
    {
      hint_ai.vtable->clean (&hint_ai);
    }
}

///@}
/** @brief Updates the game configuration based on the provided key-value pair.
 * @details This function updates the game configuration with the specified
 * parameter and value.
 */
static void
update_config (game_config_t *config, const char *key, const char *val)
{
  if (strcmp (key, "verbose") == 0)
    {
      config->verbose = (strcmp (val, "true") == 0);
    }
  else if (strcmp (key, "blitz") == 0)
    {
      config->blitz_mode = (strcmp (val, "true") == 0);
    }
  else if (strcmp (key, "ai_white") == 0)
    {
      if (strcmp (val, "false") == 0)
        {
          config->ai_white_cfg.enabled = false;
        }
      else
        {
          config->ai_white_cfg.enabled = true;
          if (strcmp (val, "true") != 0)
            {
              snprintf (config->ai_white_cfg.name, MAX_AI_STR_LEN, "%.*s",
                        MAX_AI_STR_LEN - 1, val);
            }
        }
    }
  else if (strcmp (key, "ai_black") == 0)
    {
      if (strcmp (val, "false") == 0)
        {
          config->ai_black_cfg.enabled = false;
        }
      else
        {
          config->ai_black_cfg.enabled = true;
          if (strcmp (val, "true") != 0)
            {
              snprintf (config->ai_black_cfg.name, MAX_AI_STR_LEN, "%.*s",
                        MAX_AI_STR_LEN - 1, val);
            }
        }
    }
  else if (strcmp (key, "board_size") == 0)
    {
      int new_size = atoi (val);
      if (new_size >= 4 && new_size <= 11)
        config->board_size = new_size;
      else
        printf (_ ("[ERROR] Invalid board size (4-11).\n"));
    }
  else if (strcmp (key, "time") == 0)
    {
      config->time_per_player = atoi (val);
    }
  else if (strcmp (key, "port") == 0)
    {
      config->server_port = atoi (val);
    }
  else if (strcmp (key, "language") == 0)
    {
      config->language = (strcmp (val, "fr") == 0) ? LANG_FR : LANG_EN;
    }
  else
    {
      printf (_ ("[ERROR] Unknown parameter: %s\n"), key);
      return;
    }

  printf (_ ("[CONFIG] %s updated to %s\n"), key, val);
}

int
check_blitz_timeout (void)
{
  if (g_ctx_ref && g_ctx_ref->current_game
      && g_ctx_ref->current_game->blitz_mode
      && g_ctx_ref->current_game->status == GAME_IN_PROGRESS)
    {
      update_blitz_timer (g_ctx_ref->current_game);

      if (g_ctx_ref->current_game->status != GAME_IN_PROGRESS)
        {
          rl_done = 1;
        }
    }
  return 0;
}

/** @brief Executes the quit command.
 * @details This function handles the quit command and performs necessary
 * cleanup.
 */
static void
execute_quit (CliContext *ctx)
{
  if (ctx->net_manager->current_mode != NET_MODE_NONE)
    {
      NetworkManager_Cleanup (ctx->net_manager);
      ctx->in_main_menu = true;
      ctx->in_network_match = false;
      ctx->is_away = false;
      printf (_ ("\n[NETWORK] Disconnected. Returned to local menu.\n"));
      return;
    }

  if (!ctx->in_main_menu && ctx->current_game->undo_stack.top >= 0)
    {
      printf (_ ("You are about to quit. Save current game? [y/N]: "));
      char *ans = readline (">> ");

      if (ans && (ans[0] == 'y' || ans[0] == 'Y'))
        {
          char *path = readline (_ ("Enter filename: "));
          if (path && strlen (path) > 0)
            {
              if (save_game (path, ctx->config, ctx->current_game) == 0)
                printf (_ ("Game saved successfully.\n"));
              else
                printf (_ ("[ERROR] Failed to save. Exit cancelled.\n"));
            }
          free (path);
        }
      free (ans);
    }
  ctx->game_running = false;
  printf (_ ("Goodbye!\n"));
}

static void
print_load_result (SaveReturn status, const char *filename)
{
  switch (status)
    {
    case SAVE_OK:
      printf ("Game loaded successfully!\n");
      break;
    case ERR_FILE_NOT_FOUND:
      printf ("Error: Could not open file '%s'.\n", filename);
      break;
    case ERR_BOARD_SIZE_MISMATCH:
      printf (
          _ ("Error: Declared board_size does not match the actual number of "
             "rows.\n"));
      break;
    case ERR_CORRUPT_GAME_SECTION:
      printf (
          _ ("Error: Invalid board data (illegal characters or column count "
             "mismatch).\n"));
      break;
    case ERR_CORRUPT_HISTORY_SECTION:
      printf (
          _ ("Error: Corrupted move history (invalid syntax or out-of-bounds "
             "move).\n"));
      break;
    case ERR_PREMATURE_EOF:
      printf (
          _ ("Error: Unexpected end of file (incomplete [game] section).\n"));
      break;
    case ERR_INVALID_QUEEN_COUNT:
      printf (
          _ ("Error: Cheating detected or corrupted file (exactly 4 queens "
             "per player required).\n"));
      break;
    default:
      printf (
          _ ("Error: An unknown error occurred while loading the file.\n"));
      break;
    }
}

/** @brief Handles a command entered by the user.
 * @details This function processes the command string and executes the
 * corresponding action.
 * @param input_buffer The buffer containing the user's input.
 * @param ctx The CLI context structure.
 */
void
handle_command (char *input_buffer, CliContext *ctx)
{
  if (ctx->current_game && ctx->current_game->blitz_mode
      && ctx->current_game->status != GAME_IN_PROGRESS && !ctx->in_main_menu)
    {
      cli_display_board (ctx->current_game);
      PlayerColor winner = get_winner (ctx->current_game);
      printf (_ ("\n[!] TIME ELAPSED ! Player %s won.\n"),
              (winner == PLAYER_WHITE ? _ ("White") : _ ("Black")));
      ctx->in_main_menu = true;
      rl_event_hook = NULL;
      return;
    }
  // 2. HELP & CONFIGURATION
  if (strncmp (input_buffer, "help", CMD_LEN_HELP) == 0)
    {

      printf (_ ("\n=== COMMAND LIST ===\n"));
      printf (_ ("--- Game Management ---\n"));
      printf (_ ("  new         : Start a new local game\n"));
      printf (_ ("  quit        : Exit the application\n"));
      printf (_ ("  show        : Display the board\n"));
      printf (_ ("  undo/redo   : Cancel or replay a move\n"));
      printf (_ ("  save/load   : Save or load a game state\n"));
      printf (_ ("  hint        : Get AI assistance\n"));
      printf (_ ("  set : Configure game options\n"));

      printf (_ ("\n--- Network (Server) ---\n"));
      printf (_ ("  server start [port]  : Host a new server\n"));
      printf (_ ("  server stop          : Stop hosting the server\n"));
      printf (_ ("  server status        : Show connected clients and active "
                 "games\n"));
      printf (_ ("  server list          : Discover local servers on the "
                 "network\n"));

      printf (_ ("\n--- Network (Client & Lobby) ---\n"));
      printf (_ ("  join [IP:Port] [name]: Connect to a server\n"));
      printf (_ ("  players [ID]         : List all players in the LOBBY (or "
                 "detail an ID)\n"));
      printf (_ ("  scoreboard           : Display the server's global "
                 "scoreboard\n"));
      printf (
          _ ("  ping                 : Check your latency with the server\n"));
      printf (_ ("  new <ID>             : Invite a player to a game\n"));
      printf (_ ("  accept <ID>          : Accept an invitation\n"));
      printf (_ ("  decline <ID>         : Decline an invitation\n"));
      printf (_ ("  cancel <ID>          : Cancel an invitation you sent\n"));
      printf (_ ("  away                 : Mark yourself as AWAY\n"));
      printf (_ ("  back                 : Mark yourself as BACK/IDLE\n"));

      printf (_ ("\n--- Playing a Move ---\n"));
      printf (_ ("  Format: [QueenStart]-[QueenEnd]/[ArrowEnd]  Ex: "
                 "d10-d9/d8\n"));
      printf (_ ("====================\n\n"));
      return;
    }

  // 3. NETWORK: SERVER START
  else if (strncmp (input_buffer, "server start", CMD_LEN_SERVER_START) == 0)
    {

      int port = DEFAULT_PORT;
      if (strlen (input_buffer) > CMD_LEN_SERVER_START + CMD_SPACE_OFFSET)
        sscanf (input_buffer + CMD_LEN_SERVER_START + CMD_SPACE_OFFSET, "%d",
                &port);
      if (NetworkManager_ServerStart (ctx->net_manager, port) == 0)
        {
          ctx->in_main_menu = false;
          cli_clear_screen ();

          printf (_ ("[INFO] Server started successfully on port %d.\n"),
                  port);
        }
      else
        {
          // If port is taken, warn the user and remain in menu
          printf (_ ("[ERROR] Failed to start server. Port %d might be in "
                     "use.\n"),
                  port);
        }
      return;
    }

  else if (strcmp (input_buffer, "server list") == 0)
    {

      char list_buffer[512];
      printf (_ ("\n=== Local Amazons Servers ===\n"));
      NetworkManager_GetServerList (ctx->net_manager, list_buffer,
                                    sizeof (list_buffer));
      printf ("%s", list_buffer);
      printf ("=============================\n");

      return;
    }

  // 4. NETWORK: JOIN (Full parsing)
  else if (strncmp (input_buffer, "join", CMD_LEN_JOIN) == 0)
    {

      char ip[MAX_IP_LEN + 1] = DEFAULT_IP;
      int port = DEFAULT_PORT;
      char username[MAX_USERNAME_LEN] = DEFAULT_USERNAME;

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
                  snprintf (ip, MAX_IP_LEN + 1, "%.*s", MAX_IP_LEN, arg1);
                  port = atoi (colon + 1);
                }
              else
                {
                  snprintf (ip, MAX_IP_LEN + 1, "%.*s", MAX_IP_LEN, arg1);
                }
            }
          if (parsed == 2)
            {
              snprintf (username, MAX_USERNAME_LEN, "%.*s",
                        MAX_USERNAME_LEN - 1, arg2);
            }
        }

      if (NetworkManager_ClientJoin (ctx->net_manager, ip, port) == 0)
        {
          ctx->in_main_menu = false;
          ctx->is_away = false;
          cli_clear_screen ();
          NetworkManager_SendJoin (ctx->net_manager, username);
        }
      else
        {

          printf (_ ("\n[ERROR] Connection failed. Is the server running at "
                     "%s:%d?\n"),
                  ip, port);
        }
      return;
    }

  else if (strcmp (input_buffer, "players") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (ctx->net_manager, -1);
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (ctx->net_manager, list_buffer,
                                              sizeof (list_buffer), -1);
          printf (_ ("\n=== SERVER LOBBY ===\n%s\n====================\n"),
                  list_buffer);
        }
      else
        {
          printf (_ ("[INFO] You must start or join a server first to see "
                     "players.\n"));
        }
      return;
    }
  else if (strcmp (input_buffer, "away") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendAway (ctx->net_manager);
          ctx->is_away = true;
          printf (_ ("You are now marked as AWAY.\n"));
        }
      return;
    }
  else if (strcmp (input_buffer, "back") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendBack (ctx->net_manager);
          ctx->is_away = false;
          printf (_ ("Welcome back! You are now IDLE.\n"));
        }
      return;
    }
  else if (strcmp (input_buffer, "cancel") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          if (ctx->last_invited_id > 0)
            {
              NetworkManager_SendCancel (ctx->net_manager);
              printf (_ ("[CLIENT] Invitation to player %d canceled.\n"),
                      ctx->last_invited_id);
              ctx->last_invited_id = -1;
            }
          else
            {

              printf (_ (
                  "[CLIENT] You haven't sent any invitations to cancel.\n"));
            }
        }
      else
        {

          printf (
              _ ("[INFO] Only connected clients can cancel invitations.\n"));
        }
      return;
    }
  if (strncmp (input_buffer, "quit", CMD_LEN_QUIT) == 0)
    {
      execute_quit (ctx);
      return;
    }
  // 3. NETWORK: SERVER STOP
  else if (strcmp (input_buffer, "server stop") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          NetworkManager_ShutdownServer (ctx->net_manager);
          ctx->in_main_menu = true;
          printf (_ ("\n[SERVER] Server successfully stopped. Returned to "
                     "menu.\n"));
        }
      else
        {
          printf (_ ("[MENU] Error: No server is running.\n"));
        }
      return;
    }

  // 3. NETWORK: SERVER STATUS
  else if (strcmp (input_buffer, "server status") == 0)
    {
      int clients = 0, games = 0, port = 0;
      if (NetworkManager_GetServerStatus (ctx->net_manager, &clients, &games,
                                          &port)
          == 0)
        {
          printf (_ ("\n=== SERVER STATUS ===\n"));
          printf (_ ("Status: Active\n"));
          printf (_ ("Port: %d\n"), port);
          printf (_ ("Connected clients: %d\n"), clients);
          printf (_ ("Games in progress: %d\n"), games);
          printf (_ ("=====================\n"));
        }
      else if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          printf (_ ("\n[NETWORK] Requesting server status...\n"));
        }
      else
        {
          printf (_ ("\n[ERROR] You must start or join a server first.\n"));
        }
      return;
    }
  else if (strcmp (input_buffer, "scoreboard") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendScoreboardRequest (ctx->net_manager);
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          if (system ("cat scoreboard.txt") != 0)
            {
              printf (_ ("[ERROR] Failed to read scoreboard.txt\n"));
            }
        }
      return;
    }
  else if (strcmp (input_buffer, "ping") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendPing (ctx->net_manager);
          printf (_ ("[CLIENT] PING\n"));
        }
      else
        {
          printf (_ ("[INFO] You must join a server first to ping it.\n"));
        }
      return;
    }

  else if (strncmp (input_buffer, "players", 7) == 0)
    {
      int target_id = -1;

      sscanf (input_buffer + 7, "%d", &target_id);

      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (ctx->net_manager, target_id);
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (ctx->net_manager, list_buffer,
                                              sizeof (list_buffer), target_id);
          printf ("\n=== SERVER LOBBY ===\n%s\n====================\n",
                  list_buffer);
        }
      else
        {
          printf (
              _ ("You must start or join a server first to see players.\n"));
        }
      return;
    }
  else if (strncmp (input_buffer, "accept", CMD_LEN_ACCEPT) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;

      NetworkManager_SendAccept (ctx->net_manager);
      printf (_ ("[CLIENT] accepting invitation\n"));

      return;
    }
  else if (strncmp (input_buffer, "decline", CMD_LEN_DECLINE) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;

      NetworkManager_SendDecline (ctx->net_manager);
      printf (_ ("Declined invitation from challenger.\n"));

      return;
    }

  // 5. GAME MANAGEMENT (UNDO / REDO / SAVE / LOAD / NEW)
  else if (strncmp (input_buffer, "new", CMD_LEN_NEW) == 0)
    {
      int target_id;
      if (sscanf (input_buffer + CMD_LEN_NEW, "%d", &target_id) == 1)
        {
          if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
            {
              printf (_ ("You must join a server first to invite someone.\n"));
              return;
            }
          if (NetworkManager_SendInvite (ctx->net_manager, target_id,
                                         ctx->config->board_size)
              == 0)
            {
              ctx->last_invited_id = target_id;
              printf (_ ("Invitation sent to player %d.\n"), target_id);
            }
        }
      else
        {
          printf (_ ("New local game.\n"));
          game_init (ctx->current_game);
          ctx->current_game->blitz_mode = ctx->config->blitz_mode;
          ctx->current_game->white_time_left
              = (double)ctx->config->time_per_player;
          ctx->current_game->black_time_left
              = (double)ctx->config->time_per_player;
          ctx->current_game->blitz_turn_start
              = (double)get_time_ms () / 1000.0;
          ctx->turn_start_time = time (NULL);
          ctx->in_main_menu = false;
          g_ctx_ref = ctx;
          rl_event_hook = check_blitz_timeout;
          rl_set_keyboard_input_timeout (100000);
          cli_clear_screen ();
        }
      return;
    }
  else if (strncmp (input_buffer, "undo", CMD_LEN_UNDO) == 0)
    {
      bool vs_ai = (ctx->config->ai_white_cfg.enabled
                    || ctx->config->ai_black_cfg.enabled);
      bool is_local = (ctx->net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (undo_move (ctx->current_game))
            {
              undo_move (ctx->current_game);
              printf (_ ("Move(s) undone successfully.\n"));
              cli_display_board (ctx->current_game);
            }
          else
            {
              printf (_ ("Impossible to undo.\n"));
            }
        }
      else
        {
          printf (
              _ ("The 'undo' command is only available in solo mode against "
                 "the AI.\n"));
        }
      return;
    }
  else if (strncmp (input_buffer, "redo", CMD_LEN_REDO) == 0)
    {
      bool vs_ai = (ctx->config->ai_white_cfg.enabled
                    || ctx->config->ai_black_cfg.enabled);
      bool is_local = (ctx->net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (redo_move (ctx->current_game))
            {
              redo_move (ctx->current_game);

              printf (_ ("Move(s) redone successfully.\n"));
              cli_display_board (ctx->current_game);
            }
          else
            {
              printf (_ ("Impossible to redo : no move to redo.\n"));
            }
        }
      else
        {
          printf (
              _ ("The 'redo' command is only available in solo mode against "
                 "the AI.\n"));
        }
      return;
    }
  else if (strncmp (input_buffer, "save ", CMD_LEN_SAVE) == 0)
    {
      const char *filename = input_buffer + CMD_LEN_SAVE;
      while (*filename == ' ')
        {
          filename++;
        }
      if (*filename == '\0')
        {
          printf (_ ("[ERROR] Missing filename. Usage: save <filename>\n"));
          return;
        }

      if (save_game (filename, ctx->config, ctx->current_game) == 0)
        {
          printf (_ ("Game saved successfully to: %s\n"), filename);
        }
      else
        {
          printf (_ ("[ERROR] Failed to save the game.\n"));
        }
      return;
    }
  else if (strncmp (input_buffer, "load ", CMD_LEN_LOAD) == 0)
    {
      const char *filename = input_buffer + CMD_LEN_LOAD;
      while (*filename == ' ')
        {
          filename++;
        }
      if (*filename == '\0')
        {
          printf (_ ("[ERROR] Missing filename. Usage: load <filename>\n"));
          return;
        }
      SaveReturn status = load_game (filename, ctx->config, ctx->current_game);

      if (status == SAVE_OK)
        {

          ctx->in_main_menu = false;
          if (ctx->config->blitz_mode)
            {
              ctx->current_game->blitz_mode = true;
              ctx->current_game->blitz_turn_start = (double)time (NULL);
              ctx->turn_start_time = time (NULL);
              g_ctx_ref = ctx;
              rl_event_hook = check_blitz_timeout;
              rl_set_keyboard_input_timeout (100000);
            }

          cli_clear_screen ();
          printf (_ ("Game loaded successfully from: %s\n\n"), filename);
        }
      else
        {
          print_load_result (status, filename);
        }
      return;
    }
  else if (strcmp (input_buffer, "pause") == 0)
    {
      printf (_ ("Game paused...\n"));
      return;
    }
  else if (strcmp (input_buffer, "hint") == 0)
    {
      execute_hint (ctx);
    }
  else if (strncmp (input_buffer, "show ", 5) == 0)
    {
      char *sub = input_buffer + 5;
      if (strcmp (sub, "history") == 0)
        {
          printf (_ ("=== MOVE HISTORY ===\n"));
          for (int i = 0; i <= ctx->current_game->undo_stack.top; i++)
            {
              Move m = ctx->current_game->undo_stack.data[i];
              printf ("%d: %c%d-%c%d(%c%d)\n", i + 1, m.queen_src.col + 'a',
                      m.queen_src.row + 1, m.queen_dst.col + 'a',
                      m.queen_dst.row + 1, m.arrow_dst.col + 'a',
                      m.arrow_dst.row + 1);
            }
          return;
        }
      else if (strcmp (sub, "time") == 0)
        {
          printf (_ ("Remaining time for %s: %d seconds\n"),
                  (ctx->current_game->current_player == PLAYER_WHITE
                       ? _ ("White")
                       : _ ("Black")),
                  ctx->config->time_per_player);
          return;
        }
      else if (strcmp (sub, "configuration") == 0)
        {
          display_config (ctx->config);
          return;
        }
      else if (strcmp (sub, "board") == 0)
        {
          cli_display_board (ctx->current_game);
          return;
        }
    }
  else if (strncmp (input_buffer, "set ", CMD_LEN_SET + CMD_SPACE_OFFSET) == 0)
    {
      char key[MAX_KEY_LEN], val[MAX_VAL_LEN];
      if (sscanf (input_buffer + CMD_LEN_SET + CMD_SPACE_OFFSET,
                  "%63[^=]=%63s", key, val)
          == 2)
        {
          update_config (ctx->config, key, val);
        }
      else
        {
          printf (
              _ ("[ERROR] Usage: set PARAM=VALUE (e.g., set verbose=true)\n"));
        }
      return;
    }

  // 6. GAME LOGIC (MOVES)
  Move player_move;
  if (parse_coordinates (input_buffer, &player_move) == PARSE_MOVE_SUCCESS)
    {
      if (ctx->in_main_menu)
        {
          printf (_ ("ERROR : Start a game ('new' or 'join') to play.\n"));
          return;
        }
      // --- Turn lock (Active ONLY in network mode) ---
      if (ctx->net_manager->current_mode != NET_MODE_NONE)
        {
          if (!ctx->in_network_match)
            {
              printf (
                  _ ("[ERROR] You are in the LOBBY! Use 'invite' to start a "
                     "game.\n"));
              return;
            }

          if (ctx->current_game->current_player != ctx->my_role)
            {
              printf (
                  _ ("[ERROR] It's not your turn! Wait for your opponent.\n"));
              return;
            }
        }

      if (is_move_legal (ctx->current_game, player_move))
        {
          apply_move (ctx->current_game, player_move);
          if (ctx->current_game->blitz_mode
              && ctx->current_game->status != GAME_IN_PROGRESS)
            {
              cli_display_board (ctx->current_game);
              printf (_ ("\n[!] TIME ELAPSED ! Player %s lost.\n"),
                      (ctx->current_game->status == GAME_OVER_BLACK_WINS
                           ? _ ("White")
                           : _ ("Black")));
              ctx->game_running = false;
              return;
            }

          if (ctx->net_manager->current_mode != NET_MODE_NONE)
            {
              NetworkManager_SendMove (ctx->net_manager, player_move);
            }

          ctx->timeout_sent = false;
          // We played, we reset the timer for the wait.
          ctx->turn_start_time = time (NULL);

          if (is_game_over (ctx->current_game,
                            ctx->current_game->current_player))
            {
              cli_display_board (ctx->current_game);
              PlayerColor winner = get_winner (ctx->current_game);
              printf (_ ("Player %s won !\n"),
                      (winner == PLAYER_WHITE ? _ ("White") : _ ("Black")));

              if (ctx->net_manager->current_mode != NET_MODE_NONE)
                {
                  ctx->in_network_match = false;
                  game_init (ctx->current_game);
                  printf (_ ("\n[LOBBY] Match ended. Returning to Lobby.\n"));
                  refresh_prompt (ctx);
                }
              else
                {
                  ctx->game_running = false;
                }
              rl_event_hook = NULL;
            }
        }
      else
        {
          printf (_ ("An illegal blow according to the rules.\n"));
        }
    }
  else
    {
      printf (_ ("Invalid format. Use: a1-a3/a4 format or type 'help'.\n"));
    }
}
