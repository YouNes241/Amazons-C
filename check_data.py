import numpy as np
import os
import random

def inspect_npz(directory="dataset"):
    # Discover all .npz files
    files = [f for f in os.listdir(directory) if f.endswith(".npz")]
    if not files:
        print("No .npz files found in the dataset directory.")
        return

    # Pick a random file
    target_file = os.path.join(directory, random.choice(files))
    print(f"\n[🔍] Inspecting random file: {target_file}")
    
    try:
        data = np.load(target_file)
        states = data['states']
        winners = data['winners']
        
        print("\n--- TENSOR STATS ---")
        print(f"States Shape:  {states.shape}  | Dtype: {states.dtype}")
        print(f"Winners Shape: {winners.shape}       | Dtype: {winners.dtype}")
        
        # Integers can't be NaN/Inf, so we just check if the values are within the expected [0, 1] range
        if np.issubdtype(states.dtype, np.floating):
            if np.isnan(states).any() or np.isinf(states).any():
                print("CRITICAL: NaNs or Infinities found in float states!")
        else:
            # For int8, just make sure we don't have weird values like 127
            if states.max() > 1 or states.min() < 0:
                print(f"Warning: Found values outside [0, 1] range! Max: {states.max()}, Min: {states.min()}")
            else:
                print("Tensor data is clean (all values are 0 or 1).")
            
        # Win Distribution
        unique, counts = np.unique(winners, return_counts=True)
        win_dist = dict(zip(unique, counts))
        print(f"\n--- WIN DISTRIBUTION ---")
        print(f"Total plies in file: {len(winners)}")
        print(f"White Wins (1): {win_dist.get(1, 0)}") # Removed .0 for int clarity
        print(f"Black Wins (2): {win_dist.get(2, 0)}")
        
        if len(unique) == 1:
            print("Warning: Only one color won in this entire file.")

        # Visual Board Check
        print("\n--- VISUAL CHECK (Random State) ---")
        idx = random.randint(0, len(states) - 1)
        board = states[idx]
        winner = winners[idx]
        
        print(f"Showing state index {idx} | Ultimate Winner: {'White' if winner == 1 else 'Black'}")
        print("W = White | B = Black | X = Arrow | . = Empty Square\n")
        
        for r in range(11):
            row_str = f"{r:2d} "
            for c in range(11):
                # Using integer comparisons for int8 data
                if board[r, c, 0] == 1:
                    row_str += "W "
                elif board[r, c, 1] == 1:
                    row_str += "B "
                elif board[r, c, 2] == 1:
                    row_str += "X "
                elif board[r, c, 3] == 1:
                    row_str += ". "
                else:
                    row_str += "  " 
            print(row_str)
            
    except Exception as e:
        print(f"Failed to read file! Error: {e}")

if __name__ == "__main__":
    inspect_npz()