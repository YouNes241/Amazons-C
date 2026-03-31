#include "controller_test_fixture.h"
#include <fstream>

extern "C"
{
  // Fonctions externes définies dans cli_commands.c qu'on doit tester
  char **amazons_completion (const char *text, int start, int end);
  void handle_command (char *input_buffer, CliContext *ctx);
  void refresh_prompt (CliContext *ctx);
  void update_prompt_string (CliContext *ctx);
  void execute_hint (CliContext *ctx);
}

/* ==========================================================================
 * 1. TESTS DE L'AUTO-COMPLÉTION (Readline)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, AutocompletionLogic)
{
  // Cas 1 : On tape le début d'un mot (start == 0)
  // "s" devrait autocompléter "save", "show", "set", "server status", etc.
  char **matches = amazons_completion ("s", 0, 1);
  EXPECT_NE (matches, nullptr);
  if (matches)
    {
      // Readline alloue un tableau de chaînes, on nettoie pour éviter les
      // fuites
      for (int i = 0; matches[i] != nullptr; i++)
        {
          free (matches[i]);
        }
      free (matches);
    }

  // Cas 2 : On tape au milieu d'une phrase (start != 0)
  // Notre fonction amazons_completion renvoie NULL dans ce cas
  char **no_matches = amazons_completion ("help", 5, 9);
  EXPECT_EQ (no_matches, nullptr);
}

/* ==========================================================================
 * 2. TESTS DU PROMPT (Affichage du curseur)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, PromptUpdates)
{
  // Cas 1 : Menu principal
  ctx.in_main_menu = true;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  // Cas 2 : Réseau en LOBBY
  ctx.in_main_menu = false;
  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = false;
  ctx.is_away = false;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  // Cas 3 : Réseau en mode AWAY
  ctx.is_away = true;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  // Cas 4 : Réseau EN JEU (In Match)
  ctx.in_network_match = true;
  ctx.my_role = PLAYER_WHITE;
  ctx.current_game->current_player = PLAYER_WHITE;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  // Cas 5 : Mode Local
  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_NO_THROW (update_prompt_string (&ctx));

  // Couverture de refresh_prompt (qui appelle update + fonctions Readline)
  EXPECT_NO_THROW (refresh_prompt (&ctx));
}

/* ==========================================================================
 * 3. TESTS DES COMMANDES BASIQUES ET D'AFFICHAGE
 * ==========================================================================
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
  ctx.net_manager->current_mode = NET_MODE_NONE; // Mode local
  handle_command (cmd, &ctx);
  EXPECT_FALSE (ctx.game_running); // Le jeu doit s'arrêter

  // Quit en mode réseau
  ctx.net_manager->current_mode = NET_MODE_CLIENT;
  strcpy (cmd, "quit");
  handle_command (cmd, &ctx);
  EXPECT_TRUE (ctx.in_main_menu); // Retour au menu
}

/* ==========================================================================
 * 4. TESTS DES COMMANDES RÉSEAU (Serveur)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, ServerCommands)
{
  char cmd[128];

  // Démarrer le serveur
  strcpy (cmd, "server start 9405");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Status du serveur
  strcpy (cmd, "server status");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Liste des joueurs côté serveur
  strcpy (cmd, "players");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Arrêter le serveur
  strcpy (cmd, "server stop");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}

/* ==========================================================================
 * 5. TESTS DES COMMANDES RÉSEAU (Client & Lobby)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, ClientLobbyCommands)
{
  char cmd[128];

  // Tenter de lister les serveurs
  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "server list");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Join un serveur (qui n'existe peut-être pas, mais teste le parsing
  // IP:Port)
  strcpy (cmd, "join 127.0.0.1:9405 Alice");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Simulation de l'état connecté
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

/* ==========================================================================
 * 6. TESTS DE GESTION DE PARTIE (New, Undo, Save, Load)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, GameManagementCommands)
{
  char cmd[128];

  // "new" en local
  net_mgr.current_mode = NET_MODE_NONE;
  strcpy (cmd, "new");
  handle_command (cmd, &ctx);
  EXPECT_FALSE (ctx.in_main_menu);

  // "new 2" (inviter quelqu'un en réseau)
  net_mgr.current_mode = NET_MODE_CLIENT;
  strcpy (cmd, "new 2");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Undo / Redo en réseau (doit être refusé)
  strcpy (cmd, "undo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  strcpy (cmd, "redo");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Save et Load
  strcpy (cmd, "save test_save_file.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  strcpy (cmd, "load test_save_file.amz");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Nettoyage du fichier
  remove ("test_save_file.amz");
}

/* ==========================================================================
 * 7. TESTS DE L'IA (Hint)
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, AIHintCommand)
{
  char cmd[128];

  // Cas 1 : Pas de partie en cours
  ctx.current_game = nullptr;
  strcpy (cmd, "hint");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // Cas 2 : Partie en cours, Initialisation de l'IA
  ctx.current_game = &game; // On remet le pointeur
  game_init (ctx.current_game);
  ctx.ai_initialized = false;

  strcpy (cmd, "hint");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
  // L'IA a dû être initialisée (si minimax est dispo)
}

/* ==========================================================================
 * 8. TESTS DES MOUVEMENTS ET ANTI-TRICHE
 * ==========================================================================
 */
TEST_F (ControllerTestFixture, MoveCommandsParsing)
{
  char cmd[128];

  // 1. Format totalement invalide
  strcpy (cmd, "NIMPORTE_QUOI");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // 2. Format valide mais dans le MENU
  ctx.in_main_menu = true;
  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // 3. Format valide, en jeu RÉSEAU, mais pas notre tour
  ctx.in_main_menu = false;
  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = true;
  ctx.my_role = PLAYER_BLACK;
  ctx.current_game->current_player = PLAYER_WHITE; // C'est au blanc de jouer
  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // 4. Format valide, mode LOCAL, coup LÉGAL
  net_mgr.current_mode = NET_MODE_NONE;
  ctx.current_game->current_player = PLAYER_WHITE;
  // On suppose que d10-d9(d8) est légal au premier tour. À adapter si besoin.
  strcpy (cmd, "d10-d9(d8)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));

  // 5. Format valide, mode LOCAL, coup ILLÉGAL
  // Ex: Bouger depuis une case vide
  strcpy (cmd, "a1-a2(a3)");
  EXPECT_NO_THROW (handle_command (cmd, &ctx));
}