import ctypes
import numpy as np
import os
import random
import time

# --- c lib ---
LIB_PATH = os.path.abspath('./build/src/libamazons_python.so')
lib = ctypes.CDLL(LIB_PATH)

# --- c structs ---
class Position(ctypes.Structure):
    _fields_ = [("row", ctypes.c_uint8), ("col", ctypes.c_uint8)]

class Move(ctypes.Structure):
    _fields_ = [("queen_src", Position), ("queen_dst", Position), ("arrow_dst", Position)]

class GameState(ctypes.Structure): pass
class AI_Interface(ctypes.Structure): pass

# --- c signatures ---
lib.api_create_mcts.argtypes = [ctypes.c_int, ctypes.c_int]
lib.api_create_mcts.restype = ctypes.POINTER(AI_Interface)
lib.api_init_game.argtypes = [ctypes.c_int]
lib.api_init_game.restype = ctypes.POINTER(GameState)
lib.api_get_ai_move.argtypes = [ctypes.POINTER(AI_Interface), ctypes.POINTER(GameState)]
lib.api_get_ai_move.restype = Move
lib.api_apply_move.argtypes = [ctypes.POINTER(GameState), Move]
lib.api_apply_move.restype = None
lib.api_check_winner.argtypes = [ctypes.POINTER(GameState)]
lib.api_check_winner.restype = ctypes.c_int
lib.api_export_tensor.argtypes = [ctypes.POINTER(GameState), ctypes.POINTER(ctypes.c_float)]
lib.api_export_tensor.restype = None
lib.api_free_game.argtypes = [ctypes.POINTER(GameState)]
lib.api_free_ai.argtypes = [ctypes.POINTER(AI_Interface)]

# --- geb ---
def save_batch(states, winners, filename):
    if len(states) == 0: return
    X = np.array(states, dtype=np.int8)
    Y = np.array(winners, dtype=np.int8)
    np.savez_compressed(filename, states=X, winners=Y)
    print(f"\n[SAVE] -> Saved {len(Y)} board states to {filename}\n")

def generate_games(num_games, batch_size=500): # Prefer 500/1000 batch size to save space and training time
    all_states = []
    all_winners = []

    tensor_buffer = (ctypes.c_float * (11 * 11 * 4))()
    
    existing_batches = [f for f in os.listdir("dataset_qdkd") if "batch_" in f and "partial" not in f]
    if not existing_batches:
        batch_idx = 0
    else:
        batch_idx = max([int(f.split('_')[2].split('.')[0]) for f in existing_batches]) + 1

    games_completed = batch_idx * batch_size
    print(f"Starting at Batch {batch_idx}. Total games previously finalized: {games_completed}")
    
    # Bumped to 10ms to reduce 0 moves chances
    print("Initializing AIs (10ms, 8 threads)...")
    ai_white = lib.api_create_mcts(10, 8) 
    ai_black = lib.api_create_mcts(10, 8) 
    
    try:
        while games_completed < num_games:
            current_size = random.choices([4, 5, 6, 7, 8, 9, 10, 11], weights=[1, 2, 3, 5, 9, 15, 35, 30])[0]
            state = lib.api_init_game(current_size)
            game_history = []
            corrupted_game = False 
            
            while True:
                lib.api_export_tensor(state, tensor_buffer)
                np_tensor = np.ctypeslib.as_array(tensor_buffer).reshape((4, 11, 11)).transpose(1, 2, 0).astype(np.int8).copy()
                game_history.append(np_tensor)
                
                # --- FIXED: AI Color Assignment ---
                # Since we just appended, len is 1 on White's turn (odd), and 2 on Black's turn (even).
                current_ai = ai_white if len(game_history) % 2 != 0 else ai_black
                
                move = lib.api_get_ai_move(current_ai, state)

                if move.queen_src.row == 0 and move.queen_src.col == 0 and move.queen_dst.row == 0 and move.queen_dst.col == 0:
                    print(f"Skipping Game: Null Move detected on {current_size}x{current_size}.")
                    corrupted_game = True
                    break 
                
                lib.api_apply_move(state, move)
                
                winner = lib.api_check_winner(state)
                if winner != 0:
                    break
            
            # --- CLEANUP ---
            if not corrupted_game:
                all_states.extend(game_history)
                all_winners.extend([winner] * len(game_history))
                games_completed += 1
                
                print(f"Game {games_completed}/{num_games} finished. Winner: {'White' if winner == 1 else 'Black'} ({len(game_history)} plies) | Board size: {current_size}")
                
                # Format to 00000 so the files sort nicely!
                if games_completed % batch_size == 0:
                    save_batch(all_states, all_winners, f"dataset_qdkd/amazons_batch_{batch_idx:05d}.npz")
                    all_states.clear()
                    all_winners.clear()
                    batch_idx += 1
            else:
                print("Found a bad game.\n")
            
            lib.api_free_game(state)
                
    except KeyboardInterrupt:
        print("\n[!] Execution paused. Saving remaining data...")
        
    finally:
        if len(all_states) > 0:
            ts = int(time.time())
            fn = f"dataset_qdkd/partial_{batch_idx}_{ts}.npz"
            save_batch(all_states, all_winners, fn)
            print(f"[!] Saved partial progress to {fn}")
            
        lib.api_free_ai(ai_white)
        lib.api_free_ai(ai_black)

if __name__ == "__main__":
    os.makedirs("dataset_qdkd", exist_ok=True)
    generate_games(num_games=1000000)