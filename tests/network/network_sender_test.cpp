#include "network_test_fixture.h"

/* ==========================================================================
 * CATEGORY 4: LOBBY SYSTEM API (Helpers)
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, LobbyAPIHelpersCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);

  // 1. Null/Invalid Manager Checks
  EXPECT_EQ (NetworkManager_SendInvite (nullptr, 2, 10), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendAccept (&server_mgr),
             NET_ERROR); // Server cannot accept invites
  EXPECT_EQ (NetworkManager_SendDecline (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendListRequest (nullptr, 0), NET_ERROR);

  // 2. Valid Client Calls
  EXPECT_EQ (NetworkManager_SendListRequest (&client_mgr, 0), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendInvite (&client_mgr, 2, 10), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendAccept (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendDecline (&client_mgr), NET_SUCCESS);
}

TEST_F (NetworkManagerTest, APIHelperFunctionsCoverage2)
{
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  // 2. SendJoin Coverage
  EXPECT_EQ (NetworkManager_SendJoin (nullptr, "Test"), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendJoin (&client_mgr, nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendJoin (&client_mgr, "ValidUser"), NET_SUCCESS);

  // 3. SendMove Coverage
  Move m = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
  NetworkManager none_mgr;
  NetworkManager_Init (&none_mgr, test_callbacks);
  EXPECT_EQ (NetworkManager_SendMove (&none_mgr, m), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendMove (&client_mgr, m), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendMove (&server_mgr, m), NET_SUCCESS);

  // 4. SendTimeout Coverage
  EXPECT_EQ (NetworkManager_SendTimeout (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendTimeout (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendTimeout (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendTimeout (&server_mgr), NET_SUCCESS);
}

/* ==========================================================================
 * CATEGORY 8: MISSING SENDERS & HAPPY PATH
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, MissingSendersHappyPath)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  // Vérification de toutes les nouvelles commandes
  EXPECT_EQ (NetworkManager_SendScoreboardRequest (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendAway (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendBack (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendCancel (&client_mgr), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendPing (&client_mgr), NET_SUCCESS);

  // Game over dans les deux sens (victoire/défaite)
  EXPECT_EQ (NetworkManager_SendGameOver (&client_mgr, true), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_SendGameOver (&client_mgr, false), NET_SUCCESS);

  // SendListRequest avec -1 (pour tester la branche 'else' de cette fonction)
  EXPECT_EQ (NetworkManager_SendListRequest (&client_mgr, -1), NET_SUCCESS);
}

/* ==========================================================================
 * CATEGORY 9: ROBUSTNESS - NULL POINTERS
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, SendersRobustnessNullPointers)
{
  // Appeler les fonctions avec un pointeur NULL ne doit JAMAIS causer de
  // SegFault, et doit sagement retourner NET_ERROR.
  EXPECT_EQ (NetworkManager_SendScoreboardRequest (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendAway (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendBack (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendCancel (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendPing (nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendGameOver (nullptr, true), NET_ERROR);
}

/* ==========================================================================
 * CATEGORY 10: ROBUSTNESS - INVALID STATES (Wrong Mode)
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, SendersRobustnessInvalidState)
{
  NetworkManager_ServerStart (&server_mgr, test_port);

  NetworkManager none_mgr;
  NetworkManager_Init (&none_mgr,
                       test_callbacks); // Manager vide (NET_MODE_NONE)

  // 1. Un Serveur ne devrait pas pouvoir envoyer de requêtes réservées aux
  // Clients
  EXPECT_EQ (NetworkManager_SendScoreboardRequest (&server_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendAway (&server_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendBack (&server_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendCancel (&server_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendPing (&server_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendGameOver (&server_mgr, true), NET_ERROR);

  // 2. Un Manager non-connecté ne doit rien pouvoir envoyer non plus
  EXPECT_EQ (NetworkManager_SendScoreboardRequest (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendAway (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendBack (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendCancel (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendPing (&none_mgr), NET_ERROR);
  EXPECT_EQ (NetworkManager_SendGameOver (&none_mgr, false), NET_ERROR);

  NetworkManager_Cleanup (&none_mgr);
}