#include <gtest/gtest.h>
#include <unistd.h>

extern "C"
{
#include "../include/network_common.h"
}

/* ==========================================================================
 * VARIABLES GLOBALES
 * ==========================================================================
 */
static int cb_joined_count = 0;
static int cb_disconnected_count = 0;
static int current_test_port = 9090; // 💡 NOUVEAU : Port de départ

extern "C" void
mock_on_player_joined (const char *name)
{
  (void)name;
  cb_joined_count++;
}

extern "C" void
mock_on_player_disconnected (const char *reason)
{
  (void)reason;
  cb_disconnected_count++;
}

class NetworkManagerTest : public ::testing::Test
{
protected:
  NetworkManager server_mgr;
  NetworkManager client_mgr;
  NetworkEvents test_callbacks = {};
  int test_port; // 💡 NOUVEAU : Le port attribué à CE test spécifique

  void
  SetUp () override
  {
    cb_joined_count = 0;
    cb_disconnected_count = 0;

    // À chaque nouveau test, on prend un port vierge ! (9090, puis 9091, etc.)
    test_port = current_test_port++;

    test_callbacks.OnPlayerJoined = mock_on_player_joined;
    test_callbacks.OnPlayerDisconnected = mock_on_player_disconnected;

    NetworkManager_Init (&server_mgr, test_callbacks);
    NetworkManager_Init (&client_mgr, test_callbacks);
  }

  void
  TearDown () override
  {
    NetworkManager_Cleanup (&client_mgr);
    NetworkManager_Cleanup (&server_mgr);
  }
};

/* ==========================================================================
 * TESTS F40.3
 * ==========================================================================
 */
TEST_F (NetworkManagerTest, ServerStartSuccess)
{
  // 💡 NOUVEAU : On utilise test_port au lieu de TEST_PORT
  int result = NetworkManager_ServerStart (&server_mgr, test_port);
  EXPECT_EQ (result, 0);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);
  EXPECT_NE (server_mgr.server_ctx, nullptr);
}

TEST_F (NetworkManagerTest, ClientJoinSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_EQ (result, 0);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
  EXPECT_NE (client_mgr.client_ctx, nullptr);
}

TEST_F (NetworkManagerTest, ClientJoinFailureOnWrongPort)
{
  // On met un port aléatoire où il n'y a pas de serveur
  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", 9999);
  EXPECT_EQ (result, -1);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_NONE);
}

/* ==========================================================================
 * TESTS F40.4 (Multiplexing)
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ServerTickAcceptsClient)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);

  // 💡 NOUVEAU : On autorise jusqu'à 20 essais (200ms max)
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_joined_count > 0)
        break; // Dès qu'il a répondu, on sort !
      usleep (10000);
    }

  EXPECT_EQ (cb_joined_count, 1);
}

TEST_F (NetworkManagerTest, ServerTickDetectsDisconnection)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);

  // 1. On attend l'acceptation
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_joined_count > 0)
        break;
      usleep (10000);
    }
  ASSERT_EQ (cb_joined_count, 1);

  // 2. Le client quitte
  NetworkManager_Cleanup (&client_mgr);

  // 3. On attend la détection de la déconnexion
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_disconnected_count > 0)
        break;
      usleep (10000);
    }

  EXPECT_EQ (cb_disconnected_count, 1);
}