/**
 * @file cli_commands.c
 * @brief Implementation of CLI command handling for the Amazons game.
 *
 * This file contains the implementation of the main command handler function
 * for processing user input in the command-line interface (CLI) of the
 * Amazons game. It includes logic for handling various commands related to
 * game management, network interactions, and gameplay actions, as well as
 * utilities for managing the CLI prompt and command completion.
 */

#include "cli_commands.h"
#include "ai_factory.h"
#include "cli_view.h"
#include "controller.h"
#include "game_engine.h"
#include "input.h"
#include "save.h"
#include "utils.h"
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Autocomplete ---
static char *commands_list[]
    = { "new",   "help",    "quit",         "load",          "save",
        "pause", "hint",    "undo",         "redo",          "show",
        "set",   "details", "start server", "server status", "server stop",
        "join",  "players", "accept",       "decline",       "away",
        "back",  "cancel",  "ping",         "scoreboard",    NULL };

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
  else if (ctx->net_manager != NULL
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
          if (ctx->is_away)
            {
              snprintf (prompt_str, sizeof (prompt_str), "[AWAY] >> ");
            }
          else
            {
              snprintf (prompt_str, sizeof (prompt_str), "[LOBBY] >> ");
            }
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

  // 2. HELP & CONFIGURATION
  if (strncmp (input_buffer, "help", CMD_LEN_HELP) == 0)
    {
      printf ("\n=== COMMAND LIST ===\n");
      printf ("--- Game Management ---\n");
      printf ("  new                  : Start a new local game\n");
      printf (
          "  quit                 : Exit the application or leave network\n");
      printf ("  show                 : Display the board\n");
      printf (
          "  undo/redo            : Cancel or replay a move (Local vs AI)\n");
      printf ("  save/load [filename] : Save or load a game state\n");
      printf ("  hint                 : Get AI assistance\n");
      printf ("  set / details        : Configure and display game options\n");

      printf ("\n--- Network (Server) ---\n");
      printf ("  server start [port]  : Host a new server\n");
      printf ("  server stop          : Stop hosting the server\n");
      printf ("  server status        : Show connected clients and active "
              "games\n");
      printf (
          "  server list          : Discover local servers on the network\n");

      printf ("\n--- Network (Client & Lobby) ---\n");
      printf ("  join [IP:Port] [name]: Connect to a server\n");
      printf ("  players [ID]         : List all players in the LOBBY (or "
              "detail an ID)\n");
      printf (
          "  scoreboard           : Display the server's global scoreboard\n");
      printf ("  ping                 : Check your latency with the server\n");
      printf ("  new <ID>             : Invite a player to a game\n");
      printf ("  accept <ID>          : Accept an invitation\n");
      printf ("  decline <ID>         : Decline an invitation\n");
      printf ("  cancel <ID>          : Cancel an invitation you sent\n");
      printf ("  away                 : Mark yourself as AWAY\n");
      printf ("  back                 : Mark yourself as BACK/IDLE\n");

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

      int port = DEFAULT_PORT;
      if (strlen (input_buffer) > CMD_LEN_SERVER_START + CMD_SPACE_OFFSET)
        sscanf (input_buffer + CMD_LEN_SERVER_START + CMD_SPACE_OFFSET, "%d",
                &port);
      if (NetworkManager_ServerStart (ctx->net_manager, port) == 0)
        {
          ctx->in_main_menu = false;
          cli_clear_screen ();

          printf ("[INFO] Server started successfully on port %d.\n", port);
        }
      else
        {
          // Si le port est pris, on prévient l'utilisateur et on REST dans le
          // menu
          printf ("[ERROR] Failed to start server. Port %d might be in use.\n",
                  port);
        }
      return;
    }

  else if (strcmp (input_buffer, "server list") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_NONE)
        {
          char list_buffer[512];
          printf ("\n=== Local Amazons Servers ===\n");
          NetworkManager_GetServerList (ctx->net_manager, list_buffer,
                                        sizeof (list_buffer));
          printf ("%s", list_buffer);
          printf ("=============================\n");
        }
      else
        {
          printf ("You are already connected to a network.\n");
        }
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
          ctx->in_main_menu = false;
          cli_clear_screen ();
          NetworkManager_SendJoin (ctx->net_manager, username);
        }
      else
        {

          printf (
              "\n[ERROR] Connection failed. Is the server running at %s:%d?\n",
              ip, port);
        }
      return;
    }

  else if (strcmp (input_buffer, "players") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (
              ctx->net_manager,
              -1); // -1 signifie "aucun ID fourni, on veut tout le monde"
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (
              ctx->net_manager, list_buffer, sizeof (list_buffer),
              -1); // -1 signifie "aucun ID fourni, on veut tout le monde"
          printf ("\n=== SERVER LOBBY ===\n%s\n====================\n",
                  list_buffer);
        }
      else
        {
          printf ("You must start or join a server first to see players.\n");
        }
      return;
    }
  else if (strcmp (input_buffer, "away") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendAway (ctx->net_manager);
          ctx->is_away = true;
          printf ("You are now marked as AWAY.\n");
        }
      return;
    }
  else if (strcmp (input_buffer, "back") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendBack (ctx->net_manager);
          ctx->is_away = false;
          printf ("Welcome back! You are now IDLE.\n");
        }
      return;
    }
  else if (strcmp (input_buffer, "cancel") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendCancel (ctx->net_manager, ctx->last_invited_id);
          printf ("Invitation to player %d canceled.\n", ctx->last_invited_id);
        }
      return;
    }
  if (strcmp (input_buffer, "quit") == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_NONE)
        {

          NetworkManager_Cleanup (ctx->net_manager);
          ctx->in_main_menu = true;
          ctx->in_network_match = false;
          printf ("\n[NETWORK] Disconnected. Returned to local menu.\n");
        }
      else
        {
          ctx->game_running = false;
        }
      return;
    }
  // 3. NETWORK: SERVER STOP
  else if (strcmp (input_buffer, "server stop") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          NetworkManager_ShutdownServer (ctx->net_manager);
          ctx->in_main_menu = true;
          printf (
              "\n[SERVER] Server successfully stopped. Returned to menu.\n");
        }
      else
        {
          printf ("Error: No server is running.\n");
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
          printf ("\n=== SERVER STATUS ===\n");
          printf ("Status: Active\n");
          printf ("Port: %d\n", port);
          printf ("Connected clients: %d\n", clients);
          printf ("Games in progress: %d\n", games);
          printf ("=====================\n");
        }
      else
        {
          printf ("Error: You are not running a server.\n");
        }
      return;
    }
  else if (strcmp (input_buffer, "scoreboard") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          // On crée une fonction NetworkManager_SendScoreboardRequest
          NetworkManager_SendScoreboardRequest (ctx->net_manager);
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          // Le serveur peut l'afficher directement en lisant son fichier
          if (system ("cat scoreboard.txt") != 0)
            {
              printf ("Failed to read scoreboard.txt\n");
            }
        }
      return;
    }
  else if (strcmp (input_buffer, "ping") == 0)
    {
      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendPing (ctx->net_manager);
          printf ("Client: PING\n");
        }
      else
        {
          printf ("You must join a server first to ping it.\n");
        }
      return;
    }

  else if (strncmp (input_buffer, "players", 7) == 0)
    {
      int target_id
          = -1; // -1 signifie "aucun ID fourni, on veut tout le monde"

      // On cherche un chiffre après "players"
      sscanf (input_buffer + 7, "%d", &target_id);

      if (ctx->net_manager->current_mode == NET_MODE_CLIENT)
        {
          NetworkManager_SendListRequest (ctx->net_manager,
                                          target_id); // On passe l'ID
        }
      else if (ctx->net_manager->current_mode == NET_MODE_SERVER)
        {
          char list_buffer[LOBBY_LIST_BUFFER_SIZE];
          NetworkManager_GetServerPlayerList (ctx->net_manager, list_buffer,
                                              sizeof (list_buffer),
                                              target_id); // On passe l'ID
          printf ("\n=== SERVER LOBBY ===\n%s\n====================\n",
                  list_buffer);
        }
      else
        {
          printf ("You must start or join a server first to see players.\n");
        }
      return;
    }
  else if (strncmp (input_buffer, "accept ", CMD_LEN_ACCEPT) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;

      NetworkManager_SendAccept (ctx->net_manager);
      printf ("[CLIENT] accepting invitation\n");

      return;
    }
  else if (strncmp (input_buffer, "decline", CMD_LEN_DECLINE) == 0)
    {
      if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
        return;

      NetworkManager_SendDecline (ctx->net_manager);
      printf ("Declined invitation from challenger.\n");

      return;
    }

  // 5. GAME MANAGEMENT (UNDO / REDO / SAVE / LOAD / NEW)
  else if (strncmp (input_buffer, "new", CMD_LEN_NEW) == 0)
    {
      int target_id;
      // On regarde s'il y a un ID après "new " (ex: "new 2")
      if (sscanf (input_buffer + CMD_LEN_NEW, "%d", &target_id) == 1)
        {
          if (ctx->net_manager->current_mode != NET_MODE_CLIENT)
            {
              printf ("You must join a server first to invite someone.\n");
              return;
            }
          if (NetworkManager_SendInvite (ctx->net_manager, target_id,
                                         ctx->config->board_size)
              == 0)
            {
              ctx->last_invited_id = target_id; // <-- ON MÉMORISE L'ID ICI
              printf ("Invitation sent to player %d.\n", target_id);
            }
        }
      else
        {
          // Pas d'ID ? Alors c'est une partie locale !
          printf ("New local game.\n");
          game_init (ctx->current_game);
          ctx->turn_start_time = time (NULL);
          ctx->in_main_menu = false;
          cli_clear_screen ();
        }
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
      // 1. On récupère le nom du fichier tapé par le joueur
      const char *filename = input_buffer + CMD_LEN_LOAD;

      // 2. On appelle notre nouvelle fonction de lecture
      if (load_game (filename, ctx->config, ctx->current_game) == 0)
        {
          printf ("Game loaded successfully from: %s\n", filename);
        }
      else
        {
          printf ("Failed to load the game.\n");
        }
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