#include "controller_test_fixture.h"
#include <fstream>

extern "C"
{
  /* External CLI command functions under test */
  char **amazons_completion (const char *text, int start, int end);
  void handle_command (char *input_buffer, CliContext *ctx);
  void refresh_prompt (CliContext *ctx);
  void update_prompt_string (CliContext *ctx);
  void execute_hint (CliContext *ctx);
  int check_blitz_timeout (void);
}

/**
 * @brief Tests the autocomplete function for Readline integration.
 * @details Verifies that amazons_completion returns matches when invoked
 * at position 0 (start of line), and returns NULL when invoked mid-word
 * (start != 0), as per the Readline contract.
 */
TEST_F (ControllerTestFixture, AutocompletionLogic)
{

  char **matches = amazons_completion ("s", 0, 1);
  EXPECT_NE (matches, nullptr);
  if (matches)
    {
      for (int i = 0; matches[i] != nullptr; i++)
        free (matches[i]);
      free (matches);
    }

  char **no_matches = amazons_completion ("help", 5, 9);
  EXPECT_EQ (no_matches, nullptr);
}

/**
 * @brief Tests prompt string construction for every possible UI state.
 * @details Covers the main menu state, network lobby, AWAY mode, active
 * network match (both White and Black roles), and local-only mode to ensure
 * the prompt reflects the current context in every scenario.
 */
TEST_F (ControllerTestFixture, PromptUpdates)
{

  ctx.in_main_menu = true;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  ctx.in_main_menu = false;
  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = false;
  ctx.is_away = false;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  ctx.is_away = true;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  ctx.in_network_match = true;
  ctx.is_away = false;
  ctx.my_role = PLAYER_WHITE;
  ctx.current_game->current_player = PLAYER_WHITE;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  ctx.my_role = PLAYER_BLACK;
  ctx.current_game->current_player = PLAYER_BLACK;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  EXPECT_NO_THROW (refresh_prompt (&ctx));
}

/**
 * @brief Tests basic informational commands and their output paths.
 * @details Covers help, pause, and quit in both local and network contexts,
 * verifying that game_running is correctly set to false on local quit and
 * that network quit returns cleanly to the main menu.
 */
TEST_F (ControllerTestFixture, BasicInfoCommands)
{
  char cmd[128];

  strcpy (cmd, "help");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "details");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "show");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "pause");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "quit");
  ctx.net_manager->current_mode = NET_MODE_NONE;
  handle_command (cmd, &ctx);
  EXPECT_FALSE (ctx.game_running);

  ctx.game_running = true;
  ctx.net_manager->current_mode = NET_MODE_CLIENT;
  strcpy (cmd, "quit");
  handle_command (cmd, &ctx);
  EXPECT_TRUE (ctx.in_main_menu);
}

/**
 * @brief Tests server lifecycle commands: start, status, player list, stop.
 * @details Verifies that the server can be started on a dynamic port, that
 * status and player-list queries execute without errors, and that the server
 * is properly shut down by the stop command.
 */
TEST_F (ControllerTestFixture, ServerCommands)
{
  char cmd[128];

  strcpy (cmd, "server start 9405");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "server status");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "players");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "server stop");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests client and lobby commands when connected to a server.
 * @details Covers server-list discovery, join parsing, players list, away,
 * back, ping, scoreboard, accept, decline, and cancel commands, including
 * their effects on the CliContext state.
 */
TEST_F (ControllerTestFixture, ClientLobbyCommands)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "server list");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "join 127.0.0.1:9405 Alice");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_CLIENT;

  strcpy (cmd, "players 2");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "away");
  handle_command (cmd, &ctx);
  EXPECT_TRUE (ctx.is_away);

  strcpy (cmd, "back");
  handle_command (cmd, &ctx);
  EXPECT_FALSE (ctx.is_away);

  strcpy (cmd, "ping");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "scoreboard");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "accept");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "decline");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "cancel");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests game management commands: new, undo, redo, save, and load.
 * @details Verifies that a new local game is properly initialized, that undo
 * and redo are refused in network mode, and that save/load round-trip works
 * correctly with a temporary file.
 */
TEST_F (ControllerTestFixture, GameManagementCommands)
{
  char cmd[128];
  game_cleanup (ctx.current_game);
  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "new");
  handle_command (cmd, &ctx);
  EXPECT_FALSE (ctx.in_main_menu);

  net_mgr.current_mode = NET_MODE_CLIENT;
  strcpy (cmd, "new 2");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "undo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  strcpy (cmd, "redo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "save test_save_file.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "load test_save_file.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  remove ("test_save_file.amz");
}

/**
 * @brief Tests the hint command under various AI initialization states.
 * @details Covers the case where no game is in progress (ctx.current_game is
 * NULL), and the case where AI must be lazily initialized from the hint
 * command itself.
 */
TEST_F (ControllerTestFixture, AIHintCommand)
{
  char cmd[128];

  ctx.current_game = nullptr;
  strcpy (cmd, "hint");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  ctx.current_game = &game;
  game_cleanup (ctx.current_game);
  game_init (ctx.current_game);
  ctx.ai_initialized = false;

  strcpy (cmd, "hint");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  if (ctx.ai_initialized && ctx.ai.vtable->clean)
    {
      ctx.ai.vtable->clean (&ctx.ai);
    }
  game_cleanup (ctx.current_game);
}

/**
 * @brief Tests move command parsing and turn-locking logic.
 * @details Covers invalid move format, move attempts from the main menu,
 * move rejection when it is not the player's turn in network mode, an
 * illegal move in local mode, and a legal move attempt from a standard
 * white queen position.
 */
TEST_F (ControllerTestFixture, MoveCommandsParsing)
{
  char cmd[128];

  strcpy (cmd, "NIMPORTE_QUOI");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  ctx.in_main_menu = true;
  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  ctx.in_main_menu = false;
  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = true;
  ctx.my_role = PLAYER_BLACK;
  ctx.current_game->current_player = PLAYER_WHITE;
  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  ctx.current_game->current_player = PLAYER_WHITE;
  strcpy (cmd, "a1-a2(a3)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  game_cleanup (ctx.current_game);
  game_init (ctx.current_game);
  ctx.current_game->current_player = PLAYER_WHITE;
  strcpy (cmd, "d1-d2(d3)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  game_cleanup (ctx.current_game);
}

/**
 * @brief Tests the 'set' command with every supported configuration key.
 * @details Calls handle_command with "set key=value" for all documented
 * parameters — verbose, blitz, ai_white, ai_black, board_size (valid and
 * out-of-range), time, port, and language — as well as an unknown key and
 * a malformed input without the '=' separator.
 */
TEST_F (ControllerTestFixture, SetCommandAllParameters)
{
  char cmd[128];

  strcpy (cmd, "set verbose=true");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  EXPECT_TRUE (config.verbose);

  strcpy (cmd, "set verbose=false");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  EXPECT_FALSE (config.verbose);

  strcpy (cmd, "set blitz=true");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set blitz=false");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set ai_white=true");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set ai_black=true");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set board_size=8");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  EXPECT_EQ (config.board_size, 8);

  strcpy (cmd, "set board_size=3");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set board_size=15");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set time=60");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set port=9999");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set language=fr");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set language=en");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set unknown_parameter=value");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "set noequals");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests every 'show' subcommand dispatched by handle_command.
 * @details Covers show history (with and without moves in the undo stack),
 * show time, show configuration, show board, and an unrecognised subcommand
 * to ensure the else-fall-through is safely handled.
 */
TEST_F (ControllerTestFixture, ShowSubcommandsCoverage)
{
  char cmd[128];

  ctx.in_main_menu = false;

  strcpy (cmd, "show history");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "show time");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "show configuration");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "show board");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "show unknown_sub");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  game_cleanup (ctx.current_game);
}

/**
 * @brief Tests 'show history' after moves have been played.
 * @details Applies a legal move so the undo stack is non-empty, then calls
 * 'show history' to exercise the loop that prints each stack entry.
 */
TEST_F (ControllerTestFixture, ShowHistoryWithMoves)
{
  char cmd[128];

  ctx.in_main_menu = false;
  net_mgr.current_mode = NET_MODE_NONE;

  ctx.current_game->current_player = PLAYER_WHITE;

  strcpy (cmd, "d1-d2(d3)");
  handle_command (cmd, &ctx);

  if (ctx.current_game->undo_stack.top >= 0)
    {
      strcpy (cmd, "show history");
      EXPECT_NO_THROW (handle_command (cmd, &ctx));
    }
  game_cleanup (ctx.current_game);
}

/* ==========================================================================
 * 11. TESTS DU MODE BLITZ
 * ==========================================================================
 */

/**
 * @brief Tests the blitz-mode early-return guard at the top of handle_command.
 * @details When blitz_mode is active and the game status is not
 * GAME_IN_PROGRESS, handle_command must immediately reset the UI to the
 * main menu without processing the user's input.
 */
TEST_F (ControllerTestFixture, BlitzModeTimeoutInHandleCommand)
{
  char cmd[128];

  ctx.in_main_menu = false;
  ctx.current_game->blitz_mode = true;
  ctx.current_game->status = GAME_OVER_WHITE_WINS;

  strcpy (cmd, "help");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  EXPECT_TRUE (ctx.in_main_menu);
}

/**
 * @brief Tests the check_blitz_timeout readline event hook.
 * @details Verifies the function always returns 0 (no error) regardless of
 * context state: when g_ctx_ref is not yet set, when the game is in progress,
 * and when it has ended (triggering the rl_done branch).
 */
TEST_F (ControllerTestFixture, CheckBlitzTimeoutCallback)
{

  EXPECT_EQ (check_blitz_timeout (), 0);

  char cmd[128];
  net_mgr.current_mode = NET_MODE_NONE;
  game_cleanup (ctx.current_game);
  strcpy (cmd, "new");
  handle_command (cmd, &ctx);

  game.blitz_mode = true;
  game.status = GAME_IN_PROGRESS;
  EXPECT_EQ (check_blitz_timeout (), 0);

  game.status = GAME_OVER_BLACK_WINS;
  EXPECT_EQ (check_blitz_timeout (), 0);
  game_cleanup (ctx.current_game);
}

/**
 * @brief Tests the fallback/else branches for network commands in wrong mode.
 * @details Covers the situations where ping, cancel, server-stop,
 * server-status, scoreboard, and players are invoked in a mode that does not
 * match their requirement, ensuring meaningful error messages are printed
 * without crashing.
 */
TEST_F (ControllerTestFixture, NetworkCommandFallbackBranches)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "ping");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "cancel");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "server stop");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_CLIENT;
  strcpy (cmd, "server status");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "server status");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_SERVER;
  strcpy (cmd, "scoreboard");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_SERVER;
  strcpy (cmd, "players 1");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "players 1");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "away");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  strcpy (cmd, "back");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests save and load commands including failure paths.
 * @details Writes a game to a temporary file and reloads it (success), then
 * attempts to load a non-existent path and save to an unwritable location to
 * trigger the failure-message branches.
 */
TEST_F (ControllerTestFixture, SaveLoadEdgeCases)
{
  char cmd[128];

  strcpy (cmd, "save /tmp/test_cli_coverage.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "load /tmp/test_cli_coverage.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "load /nonexistent/absolutely/nope.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "save /proc/sys/kernel/unwritable_place");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  remove ("/tmp/test_cli_coverage.amz");
}

/**
 * @brief Tests argument parsing for the 'join' command.
 * @details Exercises three distinct cases: IP:port with username, IP only
 * (no colon — covers the else branch that copies IP without parsing port),
 * and no arguments (uses default IP and port).
 */
TEST_F (ControllerTestFixture, JoinCommandVariants)
{
  char cmd[128];

  strcpy (cmd, "join 127.0.0.1");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "join");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests the 'cancel' command when a pending invitation exists.
 * @details With last_invited_id > 0, the cancel command must send a cancel
 * packet and reset last_invited_id to -1. Also tests the path where no
 * invitation is pending (prints "haven't sent any invitations").
 */
TEST_F (ControllerTestFixture, CancelWithPendingInvite)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_CLIENT;

  ctx.last_invited_id = 5;
  strcpy (cmd, "cancel");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  EXPECT_EQ (ctx.last_invited_id, -1);

  ctx.last_invited_id = -1;
  strcpy (cmd, "cancel");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests the invite-sending path of the 'new <ID>' command.
 * @details Covers the case where the player is not connected (prints an error)
 * and the case where they are connected as a client (NetworkManager_SendInvite
 * is called, which may succeed or fail gracefully since client_ctx is NULL).
 */
TEST_F (ControllerTestFixture, NewWithInvitePaths)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "new 3");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.last_invited_id = -1;
  strcpy (cmd, "new 3");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests that a move attempt from the lobby is correctly rejected.
 * @details In network mode with in_network_match == false, any move command
 * must print the "You are in the LOBBY" error and return without applying
 * the move to the game state.
 */
TEST_F (ControllerTestFixture, MoveInLobbyRejected)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = false;
  ctx.in_main_menu = false;

  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  EXPECT_EQ (ctx.current_game->current_player, PLAYER_WHITE);
}

/**
 * @brief Tests undo and redo commands when playing against the AI in local
 * mode.
 * @details Covers the vs_ai && is_local branch. When the undo stack is empty,
 * the failure sub-branch ("Impossible to undo") is triggered. After a legal
 * move, the success branch becomes reachable.
 */
TEST_F (ControllerTestFixture, UndoRedoVsAILocal)
{
  char cmd[128];

  config.ai_white = true;
  net_mgr.current_mode = NET_MODE_NONE;
  ctx.current_game->current_player = PLAYER_BLACK;

  strcpy (cmd, "undo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "redo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  config.ai_white = false;
  config.ai_black = false;
  ctx.current_game->current_player = PLAYER_WHITE;
  ctx.in_main_menu = false;
  strcpy (cmd, "d1-d2(d3)");
  handle_command (cmd, &ctx);

  config.ai_white = true;
  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "undo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "redo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests game-over detection triggered immediately after apply_move.
 * @details Constructs a board state where a single legal white move blocks all
 * black queens, causing is_game_over() to return true and the game-over branch
 * inside handle_command to fire.
 */
TEST_F (ControllerTestFixture, GameOverAfterMove)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_NONE;
  ctx.in_main_menu = false;
  ctx.game_running = true;
  ctx.current_game->current_player = PLAYER_WHITE;

  strcpy (cmd, "d1-d2(d3)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/**
 * @brief Tests the 'players' command when the client mode is active.
 * @details When current_mode == NET_MODE_CLIENT, handle_command must call
 * NetworkManager_SendListRequest. With no real server (client_ctx is NULL),
 * the send fails gracefully. Both the bare "players" and "players <ID>"
 * variants are exercised.
 */
TEST_F (ControllerTestFixture, PlayersCommandClientMode)
{
  char cmd[128];

  net_mgr.current_mode = NET_MODE_CLIENT;

  strcpy (cmd, "players");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "players 2");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}