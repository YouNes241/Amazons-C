#include "network_test_fixture.h"

/**
 * @file test_network_stats.cpp
 * @brief Unit tests for the Network Stats module, covering file I/O, updating,
 * and robustness.
 */

#include "network_test_fixture.h"
#include <cstdio>
#include <fstream>
#include <string>

class NetworkStatsTest : public ::testing::Test
{
protected:
  void
  SetUp () override
  {

    std::remove ("scoreboard.txt");
    std::remove ("scoreboard.tmp");
  }

  void
  TearDown () override
  {

    std::remove ("scoreboard.txt");
    std::remove ("scoreboard.tmp");
  }

  std::string
  ReadScoreboard ()
  {
    std::ifstream ifs ("scoreboard.txt");
    if (!ifs.is_open ())
      return "";
    std::string content ((std::istreambuf_iterator<char> (ifs)),
                         (std::istreambuf_iterator<char> ()));
    return content;
  }

  void
  WriteScoreboard (const std::string &content)
  {
    std::ofstream ofs ("scoreboard.txt");
    ofs << content;
    ofs.close ();
  }
};

TEST_F (NetworkStatsTest, UpdateNewPlayer)
{

  NetworkManager_UpdatePlayerStats ("Alice", true);
  std::string content = ReadScoreboard ();
  EXPECT_NE (content.find ("Alice 1 0 1"), std::string::npos);

  NetworkManager_UpdatePlayerStats ("Bob", false);
  content = ReadScoreboard ();
  EXPECT_NE (content.find ("Bob 0 1 1"), std::string::npos);
}

TEST_F (NetworkStatsTest, UpdateExistingPlayer)
{

  WriteScoreboard ("Charlie 5 2 7\n");

  NetworkManager_UpdatePlayerStats ("Charlie", true);
  std::string content = ReadScoreboard ();

  EXPECT_NE (content.find ("Charlie 6 2 8"), std::string::npos);
}

TEST_F (NetworkStatsTest, LoadForClientSuccess)
{
  WriteScoreboard ("Dave 10 5 15\nEve 2 2 4\n");

  ConnectedClient client;
  memset (&client, 0, sizeof (ConnectedClient));
  strncpy (client.username, "Dave", 31);

  LoadScoreboardForClient (&client);

  EXPECT_EQ (client.wins, 10);
  EXPECT_EQ (client.losses, 5);
  EXPECT_EQ (client.games_played, 15);
}

TEST_F (NetworkStatsTest, SaveScoreboardLogic)
{
  NetworkManager mgr;
  NetworkEvents ev = {};
  NetworkManager_Init (&mgr, ev);

  ASSERT_EQ (NetworkManager_ServerStart (&mgr, 0), 0);
  ASSERT_NE (mgr.server_ctx, nullptr);

  mgr.server_ctx->fds[1].fd = 100;
  mgr.server_ctx->clients[1].is_joined = true;
  strncpy (mgr.server_ctx->clients[1].username, "Frank", 31);
  mgr.server_ctx->clients[1].wins = 3;
  mgr.server_ctx->clients[1].losses = 1;
  mgr.server_ctx->clients[1].games_played = 4;

  mgr.server_ctx->fds[2].fd = 101;
  mgr.server_ctx->clients[2].is_joined = false;
  strncpy (mgr.server_ctx->clients[2].username, "Ghost", 31);

  mgr.server_ctx->fds[3].fd = 102;
  mgr.server_ctx->clients[3].is_joined = true;
  strncpy (mgr.server_ctx->clients[3].username, "Grace", 31);
  mgr.server_ctx->clients[3].wins = 0;
  mgr.server_ctx->clients[3].losses = 0;
  mgr.server_ctx->clients[3].games_played = 0;

  mgr.server_ctx->fds[4].fd = 103;
  mgr.server_ctx->clients[4].is_joined = true;
  mgr.server_ctx->clients[4].wins = 0;
  mgr.server_ctx->clients[4].losses = 0;
  mgr.server_ctx->clients[4].games_played = 0;
  strncpy (mgr.server_ctx->clients[4].username, "Alice", 31);
  mgr.server_ctx->clients[4].username[31] = '\0';

  SaveScoreboard (&mgr);

  std::string content = ReadScoreboard ();
  EXPECT_NE (content.find ("Frank 3 1 4"), std::string::npos);
  EXPECT_EQ (content.find ("Ghost"),
             std::string::npos); // The ghost is not there
  EXPECT_NE (content.find ("Grace 0 0 0"), std::string::npos);

  NetworkManager_Cleanup (&mgr);
}

TEST_F (NetworkStatsTest, Robustness_MissingFile)
{

  ConnectedClient client;
  memset (&client, 0, sizeof (client));
  strncpy (client.username, "Newbie", 31);

  EXPECT_NO_THROW (LoadScoreboardForClient (&client));
  EXPECT_EQ (client.wins, 0);
}

TEST_F (NetworkStatsTest, Robustness_CorruptedFileFormat)
{

  WriteScoreboard (
      "THIS_IS_NOT_A_VALID_FORMAT\nJust Random Words\nAlice 99 99 99");

  ConnectedClient client;
  memset (&client, 0, sizeof (client));
  strncpy (client.username, "Alice", 31);
  client.wins = 10;

  EXPECT_NO_THROW (LoadScoreboardForClient (&client));

  EXPECT_NO_THROW (NetworkManager_UpdatePlayerStats ("Bob", true));
}

TEST_F (NetworkStatsTest, Robustness_NullPointers)
{

  EXPECT_NO_THROW (SaveScoreboard (nullptr));
  EXPECT_NO_THROW (LoadScoreboardForClient (nullptr));
  EXPECT_NO_THROW (NetworkManager_UpdatePlayerStats (nullptr, true));

  NetworkManager empty_mgr;
  empty_mgr.server_ctx = nullptr;
  EXPECT_NO_THROW (SaveScoreboard (&empty_mgr));
}