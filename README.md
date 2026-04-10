# amazons-c

Une implémentation en C du jeu de plateau "Game of the Amazons" avec support CLI, GUI (GTK4), IA avancée et mode multijoueur réseau.

---

## Table des matières

1. [Présentation](#-présentation)
2. [Prérequis](#-prérequis)
3. [Installation et Compilation](#-installation-et-compilation)
   - [Compilation standard](#compilation-standard)
   - [Compilation avec TensorFlow](#compilation-avec-tensorflow)
4. [Lancement du jeu](#-lancement-du-jeu)
   - [Options de ligne de commande](#options-de-ligne-de-commande)
   - [Options de l&#39;IA](#options-de-lia)
   - [Tests de jeux](#tests-de-jeux)
5. [Modes de jeu](#-modes-de-jeu)
   - [Mode CLI (Interface en ligne de commande)](#mode-cli-interface-en-ligne-de-commande)
   - [Mode GUI (Interface graphique)](#mode-gui-interface-graphique)
   - [Mode Daemon (Serveur sans interface)](#mode-daemon-serveur-sans-interface)
6. [Commandes en jeu](#-commandes-en-jeu)
   - [Commandes générales](#commandes-générales)
   - [Commandes réseau (Serveur)](#commandes-réseau-serveur)
   - [Commandes réseau (Client et Lobby)](#commandes-réseau-client-et-lobby)
   - [Format des coups](#format-des-coups)
7. [Mode Multijoueur Réseau](#-mode-multijoueur-réseau)
   - [Scénario 1 : Lancement, Découverte et Connexion](#scénario-1--lancement-découverte-et-connexion)
   - [Scénario 2 : Gestion du Lobby et Statuts](#scénario-2--gestion-du-lobby-et-statuts)
   - [Scénario 3 : Système d&#39;invitations](#scénario-3--système-dinvitations)
   - [Scénario 4 : Gameplay, Anti-Triche et Scoreboard](#scénario-4--gameplay-anti-triche-et-scoreboard)
   - [Scénario 5 : Robustesse et Déconnexions](#scénario-5--robustesse-et-déconnexions)
   - [Scénario 6 : Cas d&#39;erreurs et Garde-fous](#scénario-6--cas-derreurs-et-garde-fous)
   - [Scénario 7 : Annulation d&#39;invitation et commande Quit](#scénario-7--annulation-dinvitation-et-commande-quit)
   - [Scénario 8 : Expiration de la découverte UDP](#scénario-8--expiration-de-la-découverte-udp)
   - [Scénario 9 : Mode Daemon / Headless](#scénario-9--mode-daemon--headless)
8. [Configuration](#-configuration)
9. [Tests](#-tests)
10. [Licence](#-licence)

---

## Présentation

**Game of the Amazons** est un jeu de stratégie abstrait pour deux joueurs se jouant sur un plateau (par défaut 10×10). Chaque joueur contrôle 4 amazones (reines) et doit bloquer les mouvements de l'adversaire en tirant des flèches après chaque déplacement.

**Caractéristiques principales :**

- Interface CLI avec autocomplétion (readline)
- Interface graphique GTK4
- Intelligence artificielle (Minimax, MCTS, Random)
- Mode multijoueur réseau avec découverte de serveurs UDP
- Mode Blitz avec timer
- Sauvegarde/Chargement de parties
- Support multilingue (FR/EN)

---

## Prérequis

Installez les dépendances suivantes sur un système basé sur Debian/Ubuntu :

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    cmake \
    git \
    libgtk-4-dev \
    libgtest-dev \
    libgmock-dev \
    libreadline-dev \
    googletest \
    doxygen \
    graphviz \
    gcovr \
    gettext \
    clang-format
```

---

## Installation et Compilation

### Compilation standard

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make
```

L'exécutable `amazons` sera créé dans le répertoire `build/`.

### Compilation avec TensorFlow

Pour activer le support Machine Learning (nécessite TensorFlow x86) :

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Coverage -DENABLE_ML=ON ..
make
```

> **Note :** Les tests seront plus longs et cette option peut ne pas fonctionner sur toutes les architectures.

---

## Lancement du jeu

```bash
./amazons [OPTIONS]
```

### Options de ligne de commande

| Option               | Description                                             |
| -------------------- | ------------------------------------------------------- |
| `-n, --size N`     | Taille du plateau (4-11, défaut: 10)                   |
| `-b, --blitz`      | Active le mode blitz (temps limité)                    |
| `-t, --time T`     | Temps de réflexion par joueur en minutes (défaut: 30) |
| `-c, --contest`    | Lance en mode tournoi (sans interface)                  |
| `-g, --gui`        | Lance en mode graphique (GTK4)                          |
| `-s, --server [P]` | Lance en mode serveur (port optionnel, défaut: 12345)  |
| `--daemon`         | Lance le serveur en arrière-plan (headless)            |
| `-d, --debug`      | Affiche les messages de débogage                       |
| `-v, --verbose`    | Active le mode verbeux                                  |
| `-h, --help`       | Affiche l'aide                                          |
| `-V, --version`    | Affiche la version du jeu                               |

### Options de l'IA

| Option                     | Description                                       |
| -------------------------- | ------------------------------------------------- |
| `--ai-white`             | Applique les options IA suivantes au joueur Blanc |
| `--ai-black`             | Applique les options IA suivantes au joueur Noir  |
| `--ai-mode M`            | Mode IA :`minimax`, `iterative`, `mcts`     |
| `--ai-minimax-depth D`   | Profondeur maximale pour minimax                  |
| `--ai-minimax-scoring S` | Fonction d'évaluation :`me`, `rth`, `qdkd` |
| `--ai-minimax-fwpruning` | Active/désactive le forward pruning              |
| `--ai-mcts-selection S`  | Sélection MCTS :`uct`, `ml`                  |
| `--ai-mcts-workers K`    | Nombre de threads `K`                           |
| `--ai-time T`            | Temps de réflexion IA en secondes                |

**Exemples :**

```bash
# Jouer contre une IA minimax (Blancs = Humain, Noirs = IA)
./amazons --ai-black --ai-mode minimax --ai-minimax-depth 4

# IA contre IA avec différentes stratégies
./amazons --ai-white --ai-mode mcts --ai-mcts-workers 5 --ai-black --ai-mode minimax

# Partie blitz de 5 minutes
./amazons -b -t 5

# Lancer un serveur sur le port 8080
./amazons -s 8080
```

## Tests de jeux

### Test : Gestion de l'historique (Undo/Redo) face à l'Intelligence Artificielle

Ce test permet de vérifier que la commande d'annulation gère correctement la boucle asynchrone de l'IA et remonte bien « jusqu'au joueur humain précédent » (Exigence F12) sans corrompre l'état de la partie.

* Lancez le jeu en configurant une IA pour jouer les Noirs avec un temps de réflexion court (pour fluidifier le test) :

  ```bash
  ./amazons --ai black --ai-mode minimax --ai-time 1000
  ```
* Démarrez la partie (`new`).
* **Coup du joueur :** Jouez un coup avec les Blancs (ex: `d1-d4/d5`).
* **Attente de l'IA :** Laissez l'IA calculer et jouer automatiquement son tour en réponse. Notez mentalement le coup qu'elle vient de jouer.
* **Annulation :** Tapez la commande `undo`.
* **Restauration du coup :** Tapez ensuite la commande `redo` pour refaire votre coup.
* **Résultats attendus :**

  * À l'étape 5 (`undo`) : L'interface ne doit pas planter. Le moteur doit exécuter une double annulation pour retirer à la fois la flèche et le déplacement de l'IA, puis la flèche et le déplacement du joueur. Le plateau revient exactement à son état d'origine et c'est de nouveau aux Blancs de jouer.
  * À l'étape 6 (`redo`) : Le coup du joueur Blanc (`d1-d4/d5`) est réappliqué sur le plateau. Le moteur redonne instantanément la main à l'IA qui va calculer une nouvelle réponse.
* Toujours sur ce même plateau, à votre tour de jeu, tapez la commande `hint`.
* **Résultats attendus :**

  * Le moteur d'IA explore l'arbre des possibles. La taille du plateau étant réduite, l'exploration est quasi instantanée.
  * L'interface affiche le meilleur coup disponible (ex: *"[ADVICE] Based on the current state: Move the queen from d2 to b2 Shoot the arrow at a1"*).
  * Le plateau de jeu ne doit pas changer. L'IA a effectué une simulation en sans corrompre l'état de la partie en cours.

### Test du mode Blitz

Ce test valide la capacité du moteur à gérer des contraintes temporelles et à interrompre une partie en cas de dépassement du temps.

* Lancez le jeu avec un temps de réflexion très court pour faciliter le test (le minimum etant 1 minute) :
  ```bash
  ./amazons -b -t 1
  ```

puis tapez *new* pour commencer.

* Jouez un coup valide directement après le lancement.
* Attendez que le temps soit écoulé.
* Tentez de jouer n'importe quel coup.
* Résultat attendu :
  * Au moment de la saisie, la logique de vérification du moteur est appelée.
  * Le moteur détecte que le timestamp actuel dépasse le temps alloué.
  * Le coup est refusé, la partie s'arrête immédiatement et le joueur adverse est déclaré vainqueur par forfait.

### Test de sauvegarde et chargement

Ce test valide la sérialisation de l'état du jeu dans un fichier, ainsi que la résilience de notre parser face à des fichiers manipulés ou corrompus.

#### Étape 1 : Cycle de sauvegarde nominal

* Lancez le jeu et démarrez une partie (`new`).
* Jouez deux ou trois coups pour modifier l'état initial (ex: `d1-d4/d5` puis `d10-d7/d6`).
* Sauvegardez la partie en tapant : `save test_partie`
* Réinitialisez le jeu en tapant `new` (le plateau redevient vierge).
* Chargez la sauvegarde en tapant : `load test_partie`
* **Résultat attendu :** Le plateau retrouve son état exact incluant la position des reines et des flèches. Le tour de jeu est restitué correctement au joueur devant agir.

### Étape 2 : Test face aux fichiers corrompus

* Laissez le jeu ouvert, et ouvrez le fichier `test_partie` généré précédemment, et enregistré dans le dossier build où se trouve l'executable du jeu..
* Insérez des éléments perturbateurs :
  * Ajoutez des lignes vides aléatoires.
  * Ajoutez un commentaire en ligne(ex: `# Inline comment`).
  * Modifiez la section [Game] de façon à ce qu'elle devienne corrompue, notamment en effaçant des lignes, ou inversement, rendant la taille du plateau de jeu incohérente avec celle déclarée dans la section [Settings].
  * Ajoutez des espaces ou des tabulations multiples entre les données.
  * Sauvegardez le fichier texte.
  * Dans le terminal du jeu, tapez à nouveau : `load test_partie.txt`
* **Résultat attendu :** L'application ne doit générer aucune erreur de segmentation (*Segfault*). Un message d'erreur devrait s'afficher expliquant pourquoi le chargement de la partie a échoué.

### Étape 3 : Fichier inexistant ou format invalide

* Tentez de charger un fichier qui n'existe pas (`load fichier_fantome.txt`) ou un fichier qui n'a rien à voir avec le jeu (ex: `load main.c`).
* **Résultat attendu :** L'interface intercepte l'erreur proprement. Un message d'erreur est affiché (ex: *"Error : Cannot find file ..."*) et le plateau actuel reste intact.

### Étape 4 : Prévention de perte de données

* Depuis votre partie en cours, jouez un nouveau coup (l'état est désormais "non sauvegardé").
* Tapez la commande `quit`.
* **Résultat attendu :** L'automate de l'interface détecte que l'état courant contient des mutations non persistées. L'action est suspendue et une confirmation est demandée au joueur : *"You are about to quit. Save current game? [y/N]: >>"*.

---

## Modes de jeu

### Mode CLI (Interface en ligne de commande)

Mode par défaut. Lance une interface textuelle interactive avec autocomplétion.

```bash
./amazons
```

### Mode GUI (Interface graphique)

Lance l'interface graphique GTK4 :

```bash
./amazons -g
```

### Mode Daemon (Serveur sans interface)

Lance un serveur en arrière-plan sans interface :

```bash
./amazons --daemon
```

Pour arrêter le serveur daemon :

```bash
killall amazons
# ou
pkill amazons
```

---

## Commandes en jeu

### Commandes générales

| Commande                 | Description                                      |
| ------------------------ | ------------------------------------------------ |
| `new`                  | Démarre une nouvelle partie locale              |
| `quit`                 | Quitte l'application                             |
| `help`                 | Affiche l'aide                                   |
| `show board`           | Affiche le plateau                               |
| `show history`         | Affiche l'historique des coups                   |
| `show time`            | Affiche le temps restant                         |
| `show configuration`   | Affiche la configuration actuelle                |
| `undo`                 | Annule le dernier coup (mode solo IA uniquement) |
| `redo`                 | Rejoue un coup annulé (mode solo IA uniquement) |
| `save <fichier>`       | Sauvegarde la partie                             |
| `load <fichier>`       | Charge une partie sauvegardée                   |
| `hint`                 | Demande un conseil à l'IA                       |
| `set <PARAM>=<VALEUR>` | Configure une option                             |

**Paramètres configurables avec `set` :**

- `verbose=true/false` : Mode verbeux
- `blitz=true/false` : Mode blitz
- `ai_white=false/minimax/mcts` : IA pour les Blancs
- `ai_black=false/minimax/mcts` : IA pour les Noirs
- `board_size=4-11` : Taille du plateau
- `time=<minutes>` : Temps par joueur
- `port=<port>` : Port du serveur
- `language=fr/en` : Langue

### Commandes réseau (Serveur)

| Commande                | Description                                        |
| ----------------------- | -------------------------------------------------- |
| `server start [port]` | Héberge un nouveau serveur                        |
| `server stop`         | Arrête le serveur                                 |
| `server status`       | Affiche les clients connectés et parties en cours |
| `server list`         | Découvre les serveurs locaux sur le réseau       |

### Commandes réseau (Client et Lobby)

| Commande                 | Description                                           |
| ------------------------ | ----------------------------------------------------- |
| `join <IP:Port> <nom>` | Se connecte à un serveur                             |
| `players [ID]`         | Liste les joueurs dans le lobby (ou détails d'un ID) |
| `scoreboard`           | Affiche le classement global du serveur               |
| `ping`                 | Vérifie la latence avec le serveur                   |
| `new <ID>`             | Invite un joueur à une partie                        |
| `accept`               | Accepte une invitation                                |
| `decline`              | Refuse une invitation                                 |
| `cancel`               | Annule une invitation envoyée                        |
| `away`                 | Se marque comme absent                                |
| `back`                 | Se marque comme disponible                            |

### Format des coups

```
[CaseDepart]-[CaseArrivee]([CaseFlèche])
```

**Exemples :**

- `d10-d9(d8)` : Déplace la reine de d10 à d9, tire une flèche en d8
- `a1-a3(a4)` : Déplace la reine de a1 à a3, tire une flèche en a4

---

## Mode Multijoueur Réseau

Pour tester l'intégralité des fonctionnalités réseau (découverte, invitations, anti-triche et multiplexage), nous recommandons d'ouvrir **3 terminaux** distincts sur la même machine.

### Scénario 1 : Lancement, Découverte et Connexion

**Terminal 1 (Serveur) :**

```bash
./amazons -s 12345
# Le serveur écoute sur le port 12345 et diffuse sa présence en UDP
```

**Terminal 2 (Client Alice) :**

```bash
./amazons
[MENU] >> server list
# Doit afficher le serveur local (UDP broadcast toutes les 10s)

[MENU] >> join 127.0.0.1:12345 Alice

[LOBBY] >> ping
# Affiche le PONG avec le calcul du RTT en millisecondes
```

**Terminal 3 (Client Bob) :**

```bash
./amazons
[MENU] >> join 127.0.0.1:12345 Bob
```

### Scénario 2 : Gestion du Lobby et Statuts

**Terminal 1 (Serveur) ou Terminal 2 (Alice) :**

```bash
[LOBBY] >> server status
# Affiche : Port 12345, 2 clients connectés, 0 parties en cours

[LOBBY] >> players
# Affiche Alice et Bob avec le statut "idle"
```

**Terminal 3 (Bob) :**

```bash
[LOBBY] >> away
# Le statut passe à AWAY

[AWAY] >> players 2
# Affiche les détails complets de Bob (ID 2), confirmant le statut AWAY

[AWAY] >> back
# Le statut redevient IDLE
```

### Scénario 3 : Système d'invitations

**Terminal 2 (Alice) :**

```bash
[LOBBY] >> new 2
# Alice passe en WAITGAME. Bob reçoit une notification.
```

**Terminal 3 (Bob) - Test du refus :**

```bash
[LOBBY] >> decline
# Alice est notifiée du refus et repasse IDLE
```

**Terminal 2 (Alice) - Nouvelle invitation :**

```bash
[LOBBY] >> new 2
```

**Terminal 3 (Bob) - Acceptation :**

```bash
[LOBBY] >> accept
# La partie se lance. Alice joue les Noirs, Bob joue les Blancs.
# Les statuts passent à INGAME.
```

> **Note sur les Timeouts d'invitation :** Envoyez une invitation (`new <ID>`) et attendez 5 minutes sans répondre. Le serveur annulera automatiquement la demande.

### Scénario 4 : Gameplay, Anti-Triche et Scoreboard

**Terminal 3 (Bob - Blancs) :**

```bash
[W] Turn: White >> d10-d9(d8)
# Le coup est validé par le serveur et envoyé à Alice. Le plateau s'affiche.
```

**Terminal 2 (Alice - Noirs) - Test Anti-Triche :**

```bash
[B] Turn: Black >> a1-z99(x0)
# Le serveur bloque le coup, ne le transmet pas à Bob,
# et renvoie une erreur "Illegal move detected" à Alice.

[LOBBY] >> scoreboard
# Le serveur affiche l'historique des victoires/défaites des joueurs.
```

### Scénario 5 : Robustesse et Déconnexions

**Test de la fermeture propre (Terminal 1 - Serveur) :**

```bash
server stop
# Alice et Bob reçoivent "Server is shutting down" et retournent au [MENU]
```

**Test du Timeout inopiné :**

1. Reconnectez un client au serveur
2. Fermez brutalement le terminal du client (croix rouge ou `kill -9`)
3. Observez le serveur : au bout de **60 secondes** exactes sans heartbeat, le serveur nettoie la connexion avec le message `Kicked for inactivity`

### Scénario 6 : Cas d'erreurs et Garde-fous

**Test 1 : Port déjà utilisé**

```bash
[MENU] >> server start 12345
# "Failed to start server. Port 12345 might be in use."
```

**Test 2 : Échec de connexion**

```bash
[MENU] >> join 127.0.0.1:9999 Alice
# "Connection failed. Is the server running at 127.0.0.1:9999?"
```

**Test 3 : Actions d'invitation invalides**

```bash
[LOBBY] >> accept
# "Error: You have no pending invitations."

[LOBBY] >> new 99
# "Error: Player not found."
```

**Test 4 : Inviter un joueur indisponible**

```bash
# Bob est en AWAY
[LOBBY] >> new 2
# "Error: Player is AWAY."
```

**Test 5 : Inviter un joueur déjà en partie**

```bash
# Alice et Bob sont INGAME
[LOBBY] >> new 1
# "Error: Player is not available."
```

### Scénario 7 : Annulation d'invitation et commande Quit

**Test de l'annulation (cancel) :**

```bash
# Terminal 2 (Alice)
[LOBBY] >> new 2
[WAITGAME] >> cancel
# Alice repasse IDLE. Bob reçoit "Invitation canceled by challenger"
```

**Test de la commande quit :**

```bash
[LOBBY] >> quit
# Alice est déconnectée proprement et retourne au [MENU] local
```

### Scénario 8 : Expiration de la découverte UDP

1. Terminal 1 (Serveur) : arrêtez le serveur (`server stop` ou `Ctrl+C`)
2. Terminal 2 (Client) : tapez `server list`. Le serveur apparaît encore (cache)
3. Attendez **30 secondes** exactement
4. Terminal 2 : retapez `server list`
   ```bash
   [MENU] >> server list
   # Le serveur a disparu ("No servers found"), prouvant le nettoyage des 30s
   ```

### Scénario 9 : Mode Daemon / Headless

```bash
./amazons --daemon
# Le serveur se lance silencieusement en tâche de fond

# Pour l'arrêter :
killall amazons
```

---

## Configuration

Le jeu utilise un fichier de configuration situé à `~/.config/amazons/config.ini`.

Vous pouvez également modifier la configuration en jeu avec la commande `set` :

```bash
set verbose=true
set board_size=8
set ai_black=minimax
set language=fr
```

---

## Tests

Les tests unitaires utilisent Google Test. Pour les exécuter :

```bash
cd build
ctest --output-on-failure
```

Pour générer un rapport de couverture :

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make
ctest
gcovr -r .. --html --html-details -o coverage.html
```

---

## Licence

Ce projet est développé dans un cadre académique.

---

## Contribution

Les contributions sont les bienvenues ! N'hésitez pas à ouvrir une issue ou une pull request.
