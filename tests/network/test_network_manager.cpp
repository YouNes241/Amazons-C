#include <gtest/gtest.h>

extern "C"
{
#include "../include/network_common.h"
}

#define TEST_PORT 9090

class NetworkManagerTest : public ::testing::Test
{
protected:
  NetworkManager server_mgr;
  NetworkManager client_mgr;
  NetworkEvents empty_callbacks = {};

  void
  SetUp () override
  {
    NetworkManager_Init (&server_mgr, empty_callbacks);
    NetworkManager_Init (&client_mgr, empty_callbacks);
  }

  void
  TearDown () override
  {
    NetworkManager_Cleanup (&client_mgr);
    NetworkManager_Cleanup (&server_mgr);
  }
};

TEST_F (NetworkManagerTest, ServerStartSuccess)
{
  int result = NetworkManager_ServerStart (&server_mgr, TEST_PORT);

  EXPECT_EQ (result, 0);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);
  EXPECT_NE (server_mgr.server_ctx, nullptr);
}

TEST_F (NetworkManagerTest, ClientJoinSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, TEST_PORT), 0);

  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", TEST_PORT);

  EXPECT_EQ (result, 0);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
  EXPECT_NE (client_mgr.client_ctx, nullptr);
}

TEST_F (NetworkManagerTest, ClientJoinFailureOnWrongPort)
{
  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", 9999);

  EXPECT_EQ (result, -1);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_NONE);
}