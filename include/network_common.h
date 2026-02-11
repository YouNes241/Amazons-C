#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <stdbool.h>

/* ==========================================================================
 * 1. Enums
 * ========================================================================== */

// Defines the state of our network machine
typedef enum {
    NET_MODE_NONE,   // Not yet initialized
    NET_MODE_SERVER, // Acts like the telephone exchange
    NET_MODE_CLIENT  // Acts like a logged-in player
} NetworkMode;


/* ==========================================================================
 * 2. Forward Declarations
 * ========================================================================== */

// The compiler is notified that these structures exist. 
// Their actual content will be defined later in `server.h` and `client.h`.
// This allows you to put pointers to them without including everything here.
typedef struct ServerContext ServerContext;
typedef struct ClientContext ClientContext;


/* ==========================================================================
 * 3. Callback Interface (The "Nervous System")
 * ========================================================================== */

// This is where we do Dependency Injection (MVC).
// The controller will fill this structure with its own functions.
typedef struct {
    // Called when one receives a move from the opponent (ex: "e2-e4")
    void (*OnMoveReceived)(const char* move_str);
    
    // Called when a new player connects to the server
    void (*OnPlayerJoined)(const char* player_name);
    
    // Called when a player leaves the game or crashes
    void (*OnPlayerDisconnected)(const char* reason);
    
    // Called when you receive an invitation to play
    void (*OnInvitationReceived)(int from_player_id);
    
    // Called in when the opponent responds to our invitation
    void (*OnInvitationResult)(bool accepted);
    
    // Called when the game really starts (with our color)
    void (*OnGameStarted)(int color);

} NetworkEvents;


/* ==========================================================================
 * 4. The Manager (The "Brain")
 * ========================================================================== */

// The front end of our network module. This is the only structure that
// the rest of the program (the Controller) needs to manipulate.
typedef struct {
    NetworkMode current_mode;
    

    // Pointers to the internal state (only one of the two will be allocated depending on the mode)
    ServerContext* server_ctx;
    ClientContext* client_ctx;
    
    // The functions to call when something happens
    NetworkEvents callbacks;
    
} NetworkManager;


/* ==========================================================================
 * 5. Prototypes of basic public functions
 * ========================================================================== */

// Initializes the manager with the callbacks provided by the Controller
void NetworkManager_Init(NetworkManager* manager, NetworkEvents callbacks);

// Cleans the memory and closes the sockets at the end of the program (Issue F43.1)
void NetworkManager_Cleanup(NetworkManager* manager);

// Runs the network machine (to be called in the game's main loop)
void NetworkManager_Tick(NetworkManager* manager);


#endif // NETWORK_COMMON_H