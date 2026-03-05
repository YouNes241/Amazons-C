#include "../../include/controller.h"
#include "../../include/input.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *commands_list[]
    = { "new",  "help", "quit", "load", "save",    "pause", "hint",
        "undo", "redo", "show", "set",  "details", NULL };

char *
command_generator (const char *text, int state)
{
  static int list_index, len;
  char *name;

  /* Si c'est le début d'une nouvelle recherche */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* On fouille dans la liste tant qu'il y a des mots */
  while ((name = commands_list[list_index++]))
    {
      /* Si le début du mot correspond à ce que l'utilisateur a tapé */
      if (strncmp (name, text, len) == 0)
        {
          /* On renvoie une copie. Readline l'affichera et fera le 'free' */
          return strdup (name);
        }
    }

  /* On n'a plus rien trouvé qui correspond */
  return NULL;
}

char **
amazons_completion (const char *text, int start, int end)
{
  /* Empêche Readline de chercher des fichiers si on ne trouve pas de commande
   */
  rl_attempted_completion_over = 1;

  /* On ne veut compléter que le premier mot de la ligne */
  /* Si start == 0, l'utilisateur est au tout début de sa saisie */
  if (start == 0)
    {
      return rl_completion_matches (text, command_generator);
    }

  return NULL;
}

void
handle_command (char *input_buffer, game_config_t *config, bool *game_running)
{
  pos_t move_pos; // structure pour stocker les coordonnées d'un coup
                  // (temporaire)

  if (strcmp (input_buffer, "quit") == 0)
    {
      *game_running = false;
    }
  else if (strcmp (input_buffer, "pause") == 0)
    {
      printf ("Jeu en pause...\n");
    }
  else if (strcmp (input_buffer, "hint") == 0)
    {
      printf ("Conseil : (IA en cours de calcul...)\n");
    }
  else if (strncmp (input_buffer, "help", 4) == 0)
    {
      printf ("Aide : new, help, quit, load, save, pause, hint, undo, "
              "redo, show, set\n");
    }
  else if (strncmp (input_buffer, "new", 3) == 0)
    {
      printf ("Nouvelle partie : %s\n", &input_buffer[3]);
    }
  else if (strncmp (input_buffer, "load ", 5) == 0)
    {
      printf ("Chargement de : %s\n", &input_buffer[5]);
    }
  else if (strncmp (input_buffer, "save ", 5) == 0)
    {
      printf ("Sauvegarde dans : %s\n", &input_buffer[5]);
    }
  else if (strncmp (input_buffer, "undo", 4) == 0)
    {
      printf ("Annulation : %s\n", &input_buffer[4]);
    }
  else if (strncmp (input_buffer, "redo", 4) == 0)
    {
      printf ("Action rejouée : %s\n", &input_buffer[4]);
    }
  else if (strncmp (input_buffer, "show ", 5) == 0)
    {
      printf ("Affichage de : %s\n", &input_buffer[5]);
    }
  else if (strncmp (input_buffer, "set ", 4) == 0)
    {
      printf ("Configuration : %s\n", &input_buffer[4]);
    }
  else if (strncmp (input_buffer, "details", 7) == 0)
    {
      display_config (config);
    }
  else
    {
      /* On vérifie si c'est un mouvement (ex: e2-e4) */
      if (parse_coordinates (input_buffer, &move_pos) == 0)
        {
          printf ("Coup valide reçu.\n");
        }
      else
        {
          fprintf (stderr, "Erreur : commande inconnue '%s'\n", input_buffer);
        }
    }
}

void
run_interactive_mode (game_config_t *config)
{
  rl_attempted_completion_function = amazons_completion;

  char *input_buffer; // buffer pour contenir les commandes de l'utilisateur
  bool game_running = true;

  while (game_running)
    {
      input_buffer = readline (">> ");

      /* Si l'utilisateur fait Ctrl+D, readline renvoie NULL */
      if (input_buffer == NULL)
        {
          printf ("\nSortie du programme.\n");
          break;
        }

      /* Si la chaîne est vide, on libère et on continue */
      if (input_buffer[0] == '\0')
        {
          free (input_buffer);
          continue;
        }

      /* On ajoute la commande à l'historique*/
      add_history (input_buffer);

      handle_command (input_buffer, config, &game_running);

      free (input_buffer);
    }
}