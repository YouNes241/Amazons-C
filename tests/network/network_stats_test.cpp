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
    // On détruit les fichiers avant le test pour avoir une ardoise magique
    std::remove ("scoreboard.txt");
    std::remove ("scoreboard.tmp");
  }

  void
  TearDown () override
  {
    // On nettoie les fichiers après le test
    std::remove ("scoreboard.txt");
    std::remove ("scoreboard.tmp");
  }

  // Helper : Lire tout le fichier
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

  // Helper : Ecrire du contenu direct dans le fichier
  void
  WriteScoreboard (const std::string &content)
  {
    std::ofstream ofs ("scoreboard.txt");
    ofs << content;
    ofs.close ();
  }
};

/* ==========================================================================
 * NORMAL BEHAVIOR (Happy Path) - Objectif 100% Couverture
 * ==========================================================================
 */

TEST_F (NetworkStatsTest, UpdateNewPlayer)
{
  // Joueur inexistant : doit créer le fichier et ajouter 1 victoire
  NetworkManager_UpdatePlayerStats ("Alice", true);
  std::string content = ReadScoreboard ();
  EXPECT_NE (content.find ("Alice 1 0 1"), std::string::npos);

  // Joueur inexistant : 1 défaite
  NetworkManager_UpdatePlayerStats ("Bob", false);
  content = ReadScoreboard ();
  EXPECT_NE (content.find ("Bob 0 1 1"), std::string::npos);
}

TEST_F (NetworkStatsTest, UpdateExistingPlayer)
{
  // On simule un joueur existant avec 5 victoires, 2 défaites (7 parties)
  WriteScoreboard ("Charlie 5 2 7\n");

  // Il gagne !
  NetworkManager_UpdatePlayerStats ("Charlie", true);
  std::string content = ReadScoreboard ();

  // Il devrait passer à 6 victoires et 8 parties
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
  NetworkManager_ServerStart (&mgr, 12345);

  // On simule deux clients actifs et un inactif
  mgr.server_ctx->clients[1].is_joined = true;
  strncpy (mgr.server_ctx->clients[1].username, "Frank", 31);
  mgr.server_ctx->clients[1].wins = 3;
  mgr.server_ctx->clients[1].losses = 1;
  mgr.server_ctx->clients[1].games_played = 4;

  mgr.server_ctx->clients[2].is_joined = false; // Ne doit pas être sauvegardé
  strncpy (mgr.server_ctx->clients[2].username, "Ghost", 31);

  mgr.server_ctx->clients[3].is_joined = true;
  strncpy (mgr.server_ctx->clients[3].username, "Grace", 31);
  mgr.server_ctx->clients[3].wins = 0;
  mgr.server_ctx->clients[3].losses = 0;
  mgr.server_ctx->clients[3].games_played = 0;

  SaveScoreboard (&mgr);

  std::string content = ReadScoreboard ();
  EXPECT_NE (content.find ("Frank 3 1 4"), std::string::npos);
  EXPECT_EQ (content.find ("Ghost"),
             std::string::npos); // Le fantôme n'y est pas
  EXPECT_NE (content.find ("Grace 0 0 0"), std::string::npos);

  NetworkManager_Cleanup (&mgr);
}

/* ==========================================================================
 * ROBUSTNESS & EDGE CASES (Les fameux "Not made for ways")
 * ==========================================================================
 */

TEST_F (NetworkStatsTest, Robustness_MissingFile)
{
  // Si le fichier n'existe pas, la fonction ne doit pas planter
  ConnectedClient client;
  memset (&client, 0, sizeof (client));
  strncpy (client.username, "Newbie", 31);

  EXPECT_NO_THROW (LoadScoreboardForClient (&client));
  EXPECT_EQ (client.wins, 0); // Rien n'a changé
}

TEST_F (NetworkStatsTest, Robustness_CorruptedFileFormat)
{
  // On met n'importe quoi dans le fichier (pas des chiffres)
  WriteScoreboard (
      "THIS_IS_NOT_A_VALID_FORMAT\nJust Random Words\nAlice 99 99 99");

  ConnectedClient client;
  memset (&client, 0, sizeof (client));
  strncpy (client.username, "Alice", 31);
  client.wins = 10; // Valeur par défaut avant lecture

  // La lecture ne doit pas exploser, même si le format est cassé
  EXPECT_NO_THROW (LoadScoreboardForClient (&client));

  // Si ton C est robuste, la mise à jour ne cassera pas tout le fichier non
  // plus
  EXPECT_NO_THROW (NetworkManager_UpdatePlayerStats ("Bob", true));
}

TEST_F (NetworkStatsTest, Robustness_NullPointers)
{
  // On passe des NULL partout pour voir si le C crashe (Segmentation Fault)
  EXPECT_NO_THROW (SaveScoreboard (nullptr));
  EXPECT_NO_THROW (LoadScoreboardForClient (nullptr));
  EXPECT_NO_THROW (NetworkManager_UpdatePlayerStats (nullptr, true));

  // Tester avec un manager non-initialisé
  NetworkManager empty_mgr;
  empty_mgr.server_ctx = nullptr;
  EXPECT_NO_THROW (SaveScoreboard (&empty_mgr));
}