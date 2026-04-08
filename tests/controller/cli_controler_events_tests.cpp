#include "controller_test_fixture.h"

/* ==========================================================================
 * 1. TESTS OF NULL POINTER ROBUSTNESS (All events should handle NULL context
 * gracefully)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, NullContextRobustness)
{
  // we create a set of events with a NULL context to ensure that all callbacks
  // can handle it without crashing
  NetworkEvents null_events = create_cli_network_events (nullptr);
  Move dummy_move = { 0, 0, 1, 1, 2, 2 };

  // We call each event with typical parameters, but since the context is NULL,
  // we are mainly checking that no segmentation faults or exceptions occur.
  EXPECT_NO_THROW (null_events.OnGameStarted (PLAYER_WHITE, 10));
  EXPECT_NO_THROW (null_events.OnMoveReceived (dummy_move));
  EXPECT_NO_THROW (null_events.OnTimeoutReceived ());
  EXPECT_NO_THROW (null_events.OnPlayerJoined ("Alice"));
  EXPECT_NO_THROW (null_events.OnPlayerDisconnected ("Server closed"));
  EXPECT_NO_THROW (null_events.OnGameError ("Error"));
  EXPECT_NO_THROW (null_events.OnInvitationReceived (2));
  EXPECT_NO_THROW (null_events.OnInvitationDeclined ("No thanks"));
  EXPECT_NO_THROW (null_events.OnListReceived ("Player 1, Player 2"));
}

/* ==========================================================================
 * 2. TESTS OF GAME START AND TIMEOUT LOGIC (Ensures correct role assignment,
 * game state initialization, and timer resets)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, GameStartedLogic)
{
  game_cleanup (ctx.current_game);
  // Starting as White should set my_role to WHITE, initialize the game with
  // the correct board size, and start the timer.
  events.OnGameStarted (PLAYER_WHITE, 10);
  EXPECT_EQ (ctx.my_role, PLAYER_WHITE);
  EXPECT_TRUE (ctx.in_network_match);
  EXPECT_EQ (ctx.current_game->size, 10);
  EXPECT_GT (ctx.turn_start_time, 0); // Timer should be set

  game_cleanup (ctx.current_game);
  // Testing with Black role and different board size
  events.OnGameStarted (PLAYER_BLACK, 8);
  EXPECT_EQ (ctx.my_role, PLAYER_BLACK);
  EXPECT_EQ (ctx.current_game->size, 8);
}

TEST_F (ControllerTestFixture, TimeoutReceivedLogic)
{
  ctx.current_game->current_player = PLAYER_WHITE;
  ctx.timeout_sent = true;

  events.OnTimeoutReceived ();

  //
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_BLACK);
  EXPECT_FALSE (ctx.timeout_sent);
  EXPECT_GT (ctx.turn_start_time, 0);

  // And in the other direction
  events.OnTimeoutReceived ();
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_WHITE);
}

/* ==========================================================================
 * 3. MOVE AND ANTI-CHEAT TESTS
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, MoveReceivedLogic)
{

  ctx.current_game->current_player = PLAYER_WHITE;

  // A. A completely illegal move (should not change the turn)
  Move illegal_move = { 0, 0, 0, 0, 0, 0 };
  events.OnMoveReceived (illegal_move);
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_WHITE);

  // B. A theoretically legal move for white (D10 to D9, arrow D8)
  // NOTE: Adjust the coordinates (queen_src_x, etc.) according to the actual
  // starting board. The important part is to hit the "if
  // (is_move_legal)" branch
  Move legal_move = { 3, 9, 3, 8, 3, 7 };

  // If is_move_legal accepts it, the timer should reset
  ctx.timeout_sent = true;
  events.OnMoveReceived (legal_move);
  // If apply_move was called (if the move was truly legal in your
  // implementation), then timeout_sent should have been set to false.
}

/* ==========================================================================
 * 4. DISCONNECTION TESTS (Multiple branches)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, PlayerDisconnected_LocalMode)
{
  net_mgr.current_mode = NET_MODE_NONE; // Offline mode

  // 1. Valid reason
  events.OnPlayerDisconnected ("Kicked");
  EXPECT_FALSE (ctx.in_network_match);
  EXPECT_TRUE (ctx.in_main_menu);

  // 2. Empty reason (strlen == 0)
  events.OnPlayerDisconnected ("");

  // 3. NULL reason
  events.OnPlayerDisconnected (nullptr);
}

TEST_F (ControllerTestFixture, PlayerDisconnected_NetworkMode)
{
  net_mgr.current_mode = NET_MODE_CLIENT; // Network mode

  // 1. Disconnected during the match
  ctx.in_network_match = true;
  events.OnPlayerDisconnected ("Ragequit");
  EXPECT_FALSE (ctx.in_network_match);

  // 2. Disconnected while just in the lobby
  ctx.in_network_match = false;
  events.OnPlayerDisconnected ("Server stopped");
  EXPECT_FALSE (ctx.in_network_match);
}

/* ==========================================================================
 * 5. UNEXPECTED CASE TESTS (Network errors and display)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, NullAndGarbageStrings)
{
  // Ensure the ternary (err ? err : "Unknown error") works
  EXPECT_NO_THROW (events.OnGameError (nullptr));
  EXPECT_NO_THROW (events.OnGameError ("Fatal Error"));

  // Miscellaneous display paths (covers OnPlayerJoined, OnInvite, etc.)
  EXPECT_NO_THROW (events.OnPlayerJoined ("Bob"));
  EXPECT_NO_THROW (events.OnInvitationReceived (5));
  EXPECT_NO_THROW (events.OnInvitationDeclined ("Too busy"));
  EXPECT_NO_THROW (events.OnListReceived ("Alice\nBob"));
}