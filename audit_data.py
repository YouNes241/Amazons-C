import numpy as np
import glob
import os
from collections import defaultdict

def audit_real_dimensions(directory="dataset"):
    files = glob.glob(os.path.join(directory, "*.npz"))
    stats = defaultdict(lambda: [0, 0, 0])

    print(f"Analyzing Channel 3 (Mask) to detect true board sizes...")

    for file in files:
        try:
            with np.load(file) as data:
                winners = data['winners']
                states = data['states'] # Shape: (N, 11, 11, 4)
                
                # We check the mask (Channel 3) of the first frame
                # to find the height and width of the 1s
                mask = states[0, :, :, 3] 
                real_h = np.max(np.where(mask.any(axis=1))[0]) + 1
                real_w = np.max(np.where(mask.any(axis=0))[0]) + 1
                
                size_key = f"{real_h}x{real_w}"
                
                stats[size_key][0] += np.sum(winners == 1)
                stats[size_key][1] += np.sum(winners == 2)
                stats[size_key][2] += len(winners)
        except: continue

    print("\n" + "═"*60)
    print(f"{'BOARD SIZE':<12} | {'TOTAL POS':<14} | {'WHITE %':<10} | {'BLACK %':<10}")
    print("─"*60)
    for size in sorted(stats.keys()):
        w, b, total = stats[size]
        print(f"{size:<12} | {total:>14,} | {w/total*100:>8.1f}% | {b/total*100:>8.1f}%")
    print("═"*60)

if __name__ == "__main__":
    audit_real_dimensions("dataset_qdkd")