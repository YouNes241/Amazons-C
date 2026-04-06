Protocole de Test du Mode Réseau
Pour tester l'intégralité des fonctionnalités réseau (découverte, invitations, anti-triche et multiplexing),
 nous vous recommandons d'ouvrir 3 terminaux distincts sur la même machine.

Préparation
Compilez le projet avec make


Scénario 1 : Lancement, Découverte et Connexion

Terminal 1 (Le Serveur) :
Testez le lancement via les arguments en ligne de commande (Daemon/Server mode).
./src/amazons-c -s 12345
// Le serveur écoute sur le port 12345 et diffuse sa présence en UDP.
Terminal 2 (Client Alice) :
Lancez le jeu normalement, puis testez la découverte UDP.
./src/amazons-c
[MENU] >> server list
// Doit afficher le serveur local (UDP broadcast toutes les 10s).
[MENU] >> join <IP_DU_SERVER>:<PORT_DU_SERVER> Alice
[LOBBY] >> ping
// Doit afficher le PONG avec le calcul du RTT en millisecondes.
Terminal 3 (Client Bob) :
./src/amazons-c
[MENU] >> join 127.0.0.1:12345 Bob



Scénario 2 : Gestion du Lobby et Statuts

Terminal 1 Serveur  ou Terminal 2 (Alice) :
Vérifiez l'état du serveur et des joueurs.
[LOBBY] >> server status
// Affiche : Port 12345, 2 clients connectés, 0 parties en cours.
[LOBBY] >> players
// Affiche Alice et Bob avec le statut "idle".
Terminal 3 (Bob) :
Testez les statuts d'absence.
[LOBBY] >> away
// Le statut passe à AWAY.
[AWAY] >> players 2
// Affiche les détails complets de Bob (ID 2), confirmant le statut AWAY.
[AWAY] >> back
// Le statut redevient IDLE.



Scénario 3 : Le système d'invitations

Terminal 2 (Alice) :
Invitez Bob (en supposant que Bob est l'ID 2).
[LOBBY] >> new 2
// Alice passe en WAITGAME. Bob reçoit une notification.
Terminal 3 (Bob) :
Testez le refus, puis acceptez une nouvelle invitation.
[LOBBY] >> decline
// Alice est notifiée du refus et repasse IDLE.
Terminal 2 (Alice) :
[LOBBY] >> new 2
Terminal 3 (Bob) :
[LOBBY] >> accept
// La partie se lance. Alice joue les Noirs, Bob joue les Blancs. Les statuts passent à INGAME.

Note sur les Timeouts d'invitation : Pour tester l'expiration, envoyez une invitation (new <ID>) et attendez 5 minutes sans répondre. Le serveur annulera automatiquement la demande.



Scénario 4 : Gameplay, Anti-Triche et Scoreboard

Terminal 3 (Bob - Blancs) :
Testez le routage d'un coup valide.
[W] Turn: White >> d10-d9(d8)
// Le coup est validé par le serveur et envoyé à Alice. Le plateau s'affiche.
Terminal 2 (Alice - Noirs) :
Testez le système Anti-Triche du serveur avec un coup illégal.
[B] Turn: Black >> a1-z99(x0)
// Le serveur bloque le coup, ne le transmet pas à Bob, et renvoie une erreur "Illegal move detected" à Alice.
[LOBBY] >> scoreboard
// Le serveur lit le fichier de sauvegarde et affiche l'historique des victoires/défaites des joueurs.



Scénario 5 : Robustesse et Déconnexions 

Test de la fermeture propre 
Sur le Terminal 1 (Serveur) :
server stop
// Alice et Bob reçoivent une notification "Server is shutting down" et sont renvoyés au [MENU].
Test du Timeout inopiné :
Reconnectez un client au serveur.
Fermez brutalement le terminal du client (cliquez sur la croix rouge ou faites un kill -9).
Observez le serveur : au bout de 60 secondes exactes sans recevoir le paquet "Heartbeat" silencieux, le serveur nettoiera la connexion avec le message Kicked for inactivity



Scénario 6 : Les cas d'erreurs et Garde-fous

Test 1 : Port déjà utilisé 
Sur le Terminal 2, essayez de démarrer un serveur sur le même port que le Terminal 1 :
[MENU] >> server start 12345
// Doit afficher un message d'erreur : "Failed to start server. Port 12345 might be in use."


Test 2 : Échec de connexion
Sur le Terminal 2, essayez de rejoindre un serveur qui n'existe pas :
[MENU] >> join 127.0.0.1:9999 Alice
// Doit afficher : "Connection failed. Is the server running at 127.0.0.1:9999?"

Test 3 : Actions d'invitation invalides (F40)
Sur le Terminal 2 (Alice) :
[LOBBY] >> accept
// Doit afficher : "Error: You have no pending invitations."
[LOBBY] >> new 99
// Doit afficher : "Error: Player not found."

Test 4 : Inviter un joueur indisponible (F40)
Sur le Terminal 3 (Bob), passez en AWAY (away).
Sur le Terminal 2 (Alice), essayez d'inviter Bob :
[LOBBY] >> new 2
// Le serveur rejette la demande : "Error: Player is AWAY."

Test 5 : Inviter un joueur déjà en partie (F40)
Pendant que Alice et Bob jouent une partie (statut INGAME) :
Sur un Terminal 4 (Client Charlie), essayez d'inviter Alice :
[LOBBY] >> new 1
// Le serveur rejette la demande : "Error: Player is not available."



Scénario 7 : Annulation d'invitation et commande Quit

Test de l'annulation (cancel) :
Sur le Terminal 2 (Alice), invitez Bob (new 2).
Sans attendre la réponse de Bob, annulez immédiatement :
[WAITGAME] >> cancel
// Alice repasse IDLE. Bob reçoit une notification "Invitation canceled by challenger" et repasse IDLE.

Test de la commande quit :
Sur le Terminal 2 (Alice) :
[LOBBY] >> quit
// Alice est déconnectée proprement du serveur et retourne au [MENU] local.
// Le serveur notifie que le client s'est déconnecté.



Scénario 8 : Expiration de la découverte UDP

Sur le Terminal 1 (Serveur), arrêtez le serveur (server stop ou Ctrl+C).
Sur le Terminal 2 (Client), tapez server list. Le serveur apparaît encore (il est en cache).
Attendez 30 secondes exactement.
Sur le Terminal 2, retapez server list :
[MENU] >> server list
// Le serveur doit avoir disparu de la liste ("No servers found"), prouvant que le nettoyage des 30s fonctionne.



Scénario 9 : Lancement en mode Daemon / Headless

Testez l'option de lancement en arrière-plan sans interface :
./src/amazons-c --daemon
// Le serveur se lance silencieusement en tâche de fond. 
// (Utilisez `killall amazons-c` pour l'arrêter ensuite).