#include "cli_commands.h"
#include "../../include/ai_factory.h"
#include "../../include/cli_view.h"
#include "../../include/controller.h"
#include "../../include/game_engine.h"
#include "../../include/input.h"
#include "../../include/save.h"
#include "../../include/utils.h"
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Autocomplete ---
static char *commands_list[]
    = { "new",    "help",    "quit",         "load", "save",
        "pause",  "hint",    "undo",         "redo", "show",
        "set",    "details", "start server", "join", "players",
        "invite", "accept",  "decline",      NULL };

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
   HELPERS UI & PROMPT
   ==========================================================================
 */

/**
 * @brief Constructs the prompt string and applies it to Readline (without
 * displaying it).
 */
void
update_prompt_string (CliContext *ctx)
{
  static char prompt_str[PROMPT_MAX_LEN]; // PROMPT_MAX_LEN
  const char *turn_str = (ctx->current_game->current_player == PLAYER_WHITE)
                             ? "White"
                             : "Black";

  if (ctx->in_main_menu)
    {
      snprintf (prompt_str, sizeof (prompt_str), "[MENU] >> ");
      rl_set_prompt (prompt_str);
      return;
    }
  // If we are on a network
  if (ctx->net_manager != NULL
      && ctx->net_manager->current_mode != NET_MODE_NONE)
    {
      if (ctx->in_network_match)
        {
          // In a game: Show Role and Turn
          const char *my_color_str
              = (ctx->my_role == PLAYER_WHITE) ? "W" : "B";
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
void
refresh_prompt (CliContext *ctx)
{
  update_prompt_string (ctx);
  rl_replace_line ("", READLINE_NO_CLEAR); // Clears the current line's memory
  rl_on_new_line ();                       // Prepares ReadLine for a new line
  rl_redisplay ();                         // Forces the display
}

/* ==========================================================================
   COMMAND LOGIC (handle_command)
   ==========================================================================
 */

void
execute_hint (CliContext *ctx)
{
  if (!ctx->current_game)
    {
      printf ("%s", "[ERROR] No game in progress to get advice.\n");
      return;
    }

  if (!ctx->ai_initialized)
    {
      printf ("%s", "[AI] Initializing advice engine...\n");
      ConfigMap empty_params = { NULL, 0 };
      ctx->ai = create_ai ("minimax", empty_params);

      if (ctx->ai.vtable == NULL)
        {
          printf ("[ERROR] %s\n", "Failed to load the advice engine.");
          return;
        }
      ctx->ai_initialized = true;
    }

  printf ("%s", "[AI] Analyzing the situation...\n");

  Move sug = ctx->ai.vtable->get_best_move (&ctx->ai, *(ctx->current_game));

  char src[POS_STRING_MAX_LEN], dst[POS_STRING_MAX_LEN],
      arr[POS_STRING_MAX_LEN];
  pos_to_string (sug.queen_src, src, sizeof (src));
  pos_to_string (sug.queen_dst, dst, sizeof (dst));
  pos_to_string (sug.arrow_dst, arr, sizeof (arr));

  printf ("\n[ADVICE] Based on the current state:\n");
  printf (" - Move the queen from %s to %s\n", src, dst);
  printf (" - Shoot the arrow at %s\n", arr);
}

void
handle_command (char *input_buffer, CliContext *ctx)
{
  // 1. QUIT
  if (strcmp (input_buffer, "quit") == 0)
    {
      ctx->game_running = false;
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
      display_config (ctx->config);
      return;
    }

  // 3. NETWORK: SERVER START
  else if (strncmp (input_buffer, "server start", CMD_LEN_SERVER_START) == 0)
    {
      ctx->in_main_menu = false;
      cli_clear_screen ();
      int port = DEFAULT_PORT;
      if (strlen (input_buffer) > CMD_LEN_SERVER_START + CMD_SPACE_OFFSET)
        sscanf (input_buffer + CMD_LEN_SERVER_START + CMD_SPACE_OFFSET, "%d",
                &port);
      NetworkManager_ServerStart (ctx->net_manager, port);
      return;
    }

  // 4. NETWORK: JOIN (Full parsing)
  else if (strncmp (input_buffer, "join", CMD_LEN_JOIN) == 0)
    {
      ctx->in_main_menu = false;
      char ip[MAX_IP_LEN + 1] = DEFAULT_IP;
      cli_clear_screen ();
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

      if (NetworkManager_ClientJoin (ctx->net_manager, ip, port) == 0)
        {
          NetworkManager_SendJoin (ctx->net_manager, username);
        }
      return;
    }

  else if (strcmp (input_buffer, "players") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (ctx->net_manager);
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (ctx->net_manager, list_buffer,
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
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        {
          printf ("You must join a server first to invite someone.\n");
          return;
        }
      int target_id;
      if (sscanf (input_buffer + CMD_LEN_INVITE, "%d", &target_id) == 1)
        {
          if (NetworkManager_SendInvite (ctx->net_manager, target_id) == 0)
            printf ("Invitation sent to player %d. Waiting for response...\n",
                    target_id);
        }
      return;
    }
  else if (strncmp (input_buffer, "accept ", CMD_LEN_ACCEPT) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;
      int challenger_id;
      if (sscanf (input_buffer + CMD_LEN_ACCEPT, "%d", &challenger_id) == 1)
        {
          if (NetworkManager_SendAccept (ctx->net_manager, challenger_id) == 0)
            printf ("Accepted invitation from player %d.\n", challenger_id);
        }
      return;
    }
  else if (strncmp (input_buffer, "decline ", CMD_LEN_DECLINE) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;
      int challenger_id;
      if (sscanf (input_buffer + CMD_LEN_DECLINE, "%d", &challenger_id) == 1)
        {
          if (NetworkManager_SendDecline (ctx->net_manager, challenger_id)
              == 0)
            printf ("Declined invitation from player %d.\n", challenger_id);
        }
      return;
    }

  // 5. GAME MANAGEMENT (UNDO / REDO / SAVE / LOAD / NEW)
  else if (strncmp (input_buffer, "new", CMD_LEN_NEW) == 0)
    {
      printf ("New local game.\n");
      game_init (ctx->current_game);
      ctx->turn_start_time = time (NULL);
      ctx->in_main_menu = false; // On quitte le menu !
      cli_clear_screen ();       // On efface le menu ASCII
      return;
    }
  else if (strncmp (input_buffer, "undo", CMD_LEN_UNDO) == 0)
    {
      bool vs_ai = (ctx->config->ai_white || ctx->config->ai_black);
      bool is_local = (ctx->net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (undo_move (ctx->current_game))
            {
              undo_move (ctx->current_game);
              printf ("Move(s) undone successfully.\n");
              cli_display_board (ctx->current_game);
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
      bool vs_ai = (ctx->config->ai_white || ctx->config->ai_black);
      bool is_local = (ctx->net_manager->current_mode == NET_MODE_NONE);

      if (vs_ai && is_local)
        {
          if (redo_move (ctx->current_game))
            {
              redo_move (ctx->current_game);

              printf ("Move(s) redone successfully.\n");
              cli_display_board (ctx->current_game);
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
      const char *filename = input_buffer + CMD_LEN_SAVE;

      if (save_game (filename, ctx->config, ctx->current_game) == 0)
        {
          printf ("Game saved successfully to: %s\n", filename);
        }
      else
        {
          printf ("Failed to save the game.\n");
        }
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
      execute_hint (ctx);
    }
  else if (strncmp (input_buffer, "show", CMD_LEN_SHOW) == 0)
    {
      cli_display_board (ctx->current_game);
      return;
    }

  // 6. GAME LOGIC (MOVES)
  Move player_move;
  if (parse_coordinates (input_buffer, &player_move) == PARSE_MOVE_SUCCESS)
    {
      if (ctx->in_main_menu)
        {
          printf ("Erreur : Commencez une partie ('new' ou 'join') avant de "
                  "jouer.\n");
          return;
        }
      // --- Turn lock (Active ONLY in network mode) ---
      if (ctx->net_manager->current_mode != NET_MODE_NONE)
        {
          if (!ctx->in_network_match)
            {
              printf ("[ERROR] You are in the LOBBY! Use 'invite' to start a "
                      "game.\n");
              return;
            }

          if (ctx->current_game->current_player != ctx->my_role)
            {
              printf ("[ERROR] It's not your turn! Wait for your opponent.\n");
              return;
            }
        }

      if (is_move_legal (ctx->current_game, player_move))
        {
          apply_move (ctx->current_game, player_move);

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
              printf ("Player %s won !\n",
                      (winner == PLAYER_WHITE ? "White" : "Black"));

              if (ctx->net_manager->current_mode != NET_MODE_NONE)
                {
                  ctx->in_network_match = false;
                  game_init (ctx->current_game);
                  printf ("\n[LOBBY] Match ended. Returning to Lobby.\n");
                  refresh_prompt (ctx);
                }
              else
                {
                  ctx->game_running = false;
                }
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