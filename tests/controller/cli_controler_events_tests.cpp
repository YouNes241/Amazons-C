#include "controller_test_fixture.h"

/* ==========================================================================
 * 1. TEST DE ROBUSTESSE EXTRÊME : CONTEXTE NULL
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, NullContextRobustness)
{
  // On écrase le pointeur statique avec NULL
  NetworkEvents null_events = create_cli_network_events (nullptr);
  Move dummy_move = { 0, 0, 1, 1, 2, 2 };

  // Aucun de ces appels ne doit faire crasher le programme (Segfault)
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
 * 2. TESTS DE JEU (GameStarted & Timeout)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, GameStartedLogic)
{
  game_cleanup (ctx.current_game);
  // Test Démarrage en tant que Blanc
  events.OnGameStarted (PLAYER_WHITE, 10);
  EXPECT_EQ (ctx.my_role, PLAYER_WHITE);
  EXPECT_TRUE (ctx.in_network_match);
  EXPECT_EQ (ctx.current_game->size, 10);
  EXPECT_GT (ctx.turn_start_time, 0); // Timer activé

  game_cleanup (ctx.current_game);
  // Test Démarrage en tant que Noir
  events.OnGameStarted (PLAYER_BLACK, 8);
  EXPECT_EQ (ctx.my_role, PLAYER_BLACK);
  EXPECT_EQ (ctx.current_game->size, 8);
}

TEST_F (ControllerTestFixture, TimeoutReceivedLogic)
{
  ctx.current_game->current_player = PLAYER_WHITE;
  ctx.timeout_sent = true;

  events.OnTimeoutReceived ();

  // Vérifie le switch de joueur et le reset du chrono
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_BLACK);
  EXPECT_FALSE (ctx.timeout_sent);
  EXPECT_GT (ctx.turn_start_time, 0);

  // Et dans l'autre sens
  events.OnTimeoutReceived ();
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_WHITE);
}

/* ==========================================================================
 * 3. TESTS DES MOUVEMENTS ET ANTI-TRICHE
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, MoveReceivedLogic)
{

  ctx.current_game->current_player = PLAYER_WHITE;

  // A. Un coup complètement illégal (ne doit pas changer le tour)
  Move illegal_move = { 0, 0, 0, 0, 0, 0 };
  events.OnMoveReceived (illegal_move);
  EXPECT_EQ (ctx.current_game->current_player, PLAYER_WHITE);

  // B. Un coup théoriquement légal pour les blancs (D10 vers D9, flèche D8)
  // NOTE: Ajuste les coordonnées (queen_src_x, etc.) selon l'état initial réel
  // de ton plateau. L'important c'est de passer dans la branche "if
  // (is_move_legal)"
  Move legal_move = { 3, 9, 3, 8, 3, 7 };

  // Si is_move_legal l'accepte, le timer doit être reset
  ctx.timeout_sent = true;
  events.OnMoveReceived (legal_move);
  // Si la fonction apply_move a été appelée (si le coup était vraiment légal
  // dans ton implémentation), alors timeout_sent est passé à false.
}

/* ==========================================================================
 * 4. TESTS DE DÉCONNEXION (Branches multiples)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, PlayerDisconnected_LocalMode)
{
  net_mgr.current_mode = NET_MODE_NONE; // Mode hors ligne

  // 1. Motif valide
  events.OnPlayerDisconnected ("Kicked");
  EXPECT_FALSE (ctx.in_network_match);
  EXPECT_TRUE (ctx.in_main_menu);

  // 2. Motif vide (strlen == 0)
  events.OnPlayerDisconnected ("");

  // 3. Motif NULL
  events.OnPlayerDisconnected (nullptr);
}

TEST_F (ControllerTestFixture, PlayerDisconnected_NetworkMode)
{
  net_mgr.current_mode = NET_MODE_CLIENT; // Mode réseau

  // 1. Déconnecté en plein match
  ctx.in_network_match = true;
  events.OnPlayerDisconnected ("Ragequit");
  EXPECT_FALSE (ctx.in_network_match);

  // 2. Déconnecté alors qu'on était juste dans le lobby
  ctx.in_network_match = false;
  events.OnPlayerDisconnected ("Server stopped");
  EXPECT_FALSE (ctx.in_network_match);
}

/* ==========================================================================
 * 5. TESTS DES CAS IMPRÉVUS (Erreurs réseau et affichage)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, NullAndGarbageStrings)
{
  // On s'assure que le ternaire (err ? err : "Unknown error") fonctionne
  EXPECT_NO_THROW (events.OnGameError (nullptr));
  EXPECT_NO_THROW (events.OnGameError ("Fatal Error"));

  // Affichages divers (Couvre OnPlayerJoined, OnInvite, etc.)
  EXPECT_NO_THROW (events.OnPlayerJoined ("Bob"));
  EXPECT_NO_THROW (events.OnInvitationReceived (5));
  EXPECT_NO_THROW (events.OnInvitationDeclined ("Too busy"));
  EXPECT_NO_THROW (events.OnListReceived ("Alice\nBob"));
}