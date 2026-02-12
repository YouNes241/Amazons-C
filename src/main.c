#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>  
#include <stdbool.h> 
#include "config.h"
#include "input.h"


void
init_default_config (game_config_t *config)
{
  config->board_size = DEFAULT_BOARD_SIZE;
  config->mode = MODE_INTERACTIVE;
  config->blitz_mode = false;
  config->time_per_player = DEFAULT_TIME_BLITZ;
  config->ai_enabled = false;
  config->verbose = false;
  config->language = LANG_FR;
}

void
display_config (game_config_t *config)
{
  printf ("\n--- CONFIGURATION ACTUELLE ---\n");
  printf ("Mode de jeu       : ");

  switch (config->mode)
    {
    case MODE_INTERACTIVE:
      printf ("Interactif \n");
      break;
    case MODE_CONTEST:
      printf ("Contest \n");
      break;
    case MODE_GUI:
      printf ("GUI \n");
      break;
    default:
      printf ("Inconnu\n");
      break;
    }

  printf ("\n--- CONFIGURATION ACTUELLE ---\n");
  printf ("Taille du plateau : %d x %d\n", config->board_size, config->board_size);
  printf ("Mode Blitz        : %s\n", config->blitz_mode ? "Activé" : "Désactivé");
  printf ("Temps par joueur  : %d s\n", config->time_per_player);
  printf ("IA activée        : %s\n", config->ai_enabled ? "Oui" : "Non");
  printf ("Mode Verbose      : %s\n", config->verbose ? "Activé" : "Désactivé");
  printf ("Langue : %s\n", config->language == LANG_FR ? "Français" : "Anglais");
  printf ("------------------------------\n");
}

int
main (int argc, char *argv[])
{
  game_config_t config;
  int opt;
  int option_index = 0;

  
  static struct option long_options[] =
    {
      {"size",    required_argument, 0, 'n'},
      {"blitz",   no_argument,       0, 'b'},
      {"time",    required_argument, 0, 't'},
      {"contest", no_argument,       0, 'c'},
      {"gui",     no_argument,       0, 'g'},
      {"ai",      optional_argument, 0, 'a'},
      {"debug",   no_argument,       0, 'd'},
      {"verbose", no_argument,       0, 'v'},
      {"help",    no_argument,       0, 'h'},
      {"version",    no_argument,       0, 'V'},
      {0, 0, 0, 0}
    };

  init_default_config (&config);

  // getopt_long parcourt les arguments de la ligne de commande et les compare aux options définies dans long_options. 
  //il prend en arguments argc et argv, qui sont les paramètres standard de la fonction main représentant le nombre d'arguments et les arguments eux-mêmes.
  //ainsi que une chaîne de caractères "n:bt:cga::dvVh" qui indique les options courtes acceptées par le programme.
  while ((opt = getopt_long (argc, argv, "n:bt:cga::dvVh", long_options, &option_index)) != -1)
    {
      switch (opt)
        {
        case 'n':
          config.board_size = atoi (optarg);
          if (config.board_size < MIN_BOARD_SIZE || config.board_size > MAX_BOARD_SIZE)
            {
              fprintf (stderr, "Erreur : La taille du plateau doit etre entre %d et %d.\n", 
                       MIN_BOARD_SIZE, MAX_BOARD_SIZE);
              return EXIT_FAILURE;
            }
          break;

        case 'b':
          config.blitz_mode = true;
          break;

        case 't':
          config.time_per_player = atoi (optarg);
          if (config.time_per_player <= 0 || config.time_per_player > 1800) 
            { 
              fprintf (stderr, "Erreur : Le temps par joueur doit etre un entier positif entre 1 et 1800 secondes.\n"); 
              return EXIT_FAILURE; 
            }
          break;

        case 'c':
          config.mode = MODE_CONTEST;
          break;

        case 'g':
          config.mode = MODE_GUI;
          break;

        case 'a':
          config.ai_enabled = true;
          break;

        case 'd':
        case 'v':
          
          config.verbose = true;
          break;

        case 'V':
          printf("amazons-c version %s\n", VERSION);
          return EXIT_SUCCESS;

        case 'h':
          printf ("Usage: ./amazons [OPTIONS]\n");
          printf ("  -n, --size N    Taille du plateau (%d-%d, defaut: %d)\n", 
                  MIN_BOARD_SIZE, MAX_BOARD_SIZE, DEFAULT_BOARD_SIZE);
          printf ("  -b, --blitz     Activer le mode blitz\n");
          printf ("  -t, --time T    Temps par joueur en secondes\n");
          printf ("  -c, --contest   Lancer en mode contest\n");
          printf ("  -g, --gui       Lancer en mode graphique\n");
          printf ("  -a, --ai [C]    Activer l'IA\n");
          printf ("  -d, --debug     Afficher les messages de debug\n");
          printf ("  -v, --verbose   Activer le mode bavard\n");
          printf ("  -h, --help      Afficher cette aide\n");
          printf ("  -V, --version   Affiche la version du jeu\n");
          return EXIT_SUCCESS;

          default:
          return EXIT_FAILURE;
        }
    }


  if (optind < argc)
    {
      fprintf (stderr, "Erreur : argument inconnu '%s'\n", argv[optind]);
      return EXIT_FAILURE;
    }

{
    char input_buffer[256]; //buffer pour contenir les commandes de l'utilisateur
    bool game_running = true;
    pos_t move_pos; //structure pour stocker les coordonnées d'un coup  (temporaire)

    while (game_running)
      {
        printf (">> ");
        fflush (stdout);

        if (fgets (input_buffer, sizeof (input_buffer), stdin) == NULL) //elle récupère l'entrée de l'utilisateur depuis l'entrée standard et le stock dans le buffer
          break;

        input_buffer[strcspn (input_buffer, "\n")] = 0; //elle supprime le caractère de retour à la ligne

        if (input_buffer[0] == '\0') //si la chaîne est vide, on attend 
          continue;

        //commandes sans arguments donc on compare l'entrée complète d'où strcmp
        if (strcmp (input_buffer, "quit") == 0)
          {
            game_running = false;
          }
        else if (strcmp (input_buffer, "pause") == 0)
          {
            printf ("Jeu en pause...\n");
          }
        else if (strcmp (input_buffer, "hint") == 0)
          {
            printf ("Conseil : (IA en cours de calcul...)\n");
          }
        
        // commandes avec des arguments optionnels ou pas, on compare juste le début de la chaîne d'où strncmp
        else if (strncmp (input_buffer, "help", 4) == 0) 
          {
            printf ("Aide : new, help, quit, load, save, pause, hint, undo, redo, show, set\n");
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
            display_config (&config);
          }
        
        else
          {
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
  }

  return EXIT_SUCCESS;
}