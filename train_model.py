import tensorflow as tf
from tensorflow import keras
import numpy as np
import glob
import random
import matplotlib.pyplot as plt

# NN
model = keras.Sequential([
    keras.layers.Input(shape=(11, 11, 4)),
    keras.layers.Flatten(),
    keras.layers.Dense(64, activation='relu'), # Hidden layer for slightly better parameters
    keras.layers.Dense(1, activation='sigmoid')
])
model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])

# Splitting
all_files = sorted(glob.glob("dataset_qdkd/*.npz"))
random.shuffle(all_files)

split_idx = int(len(all_files) * 0.8)
train_files = all_files[:split_idx]
val_files = all_files[split_idx:]

history = {'train_loss': [], 'val_loss': [], 'train_acc': [], 'val_acc': []}
NUM_EPOCHS = 50

# Training
print(f"Training on {len(train_files)} files, Validating on {len(val_files)} files.")

for epoch in range(NUM_EPOCHS):
    random.shuffle(train_files)
    
    # --- Training Phase ---
    epoch_train_loss = []
    for file in train_files[:100]: # Sample 100 files per epoch to keep it fast
        with np.load(file) as data:
            X, y = data['states'].astype('float32'), np.where(data['winners'] == 1, 1.0, 0.0).astype('float32')
            metrics = model.train_on_batch(X, y)
            epoch_train_loss.append(metrics[0])
    
    # --- Validation Phase ---
    epoch_val_loss = []
    epoch_val_acc = []
    for file in val_files[:20]: # Check against 20 different files
        with np.load(file) as data:
            X, y = data['states'].astype('float32'), np.where(data['winners'] == 1, 1.0, 0.0).astype('float32')
            val_metrics = model.test_on_batch(X, y) # test_on_batch doesn't update weights
            epoch_val_loss.append(val_metrics[0])
            epoch_val_acc.append(val_metrics[1])

    # Record average for the epoch
    history['train_loss'].append(np.mean(epoch_train_loss))
    history['val_loss'].append(np.mean(epoch_val_loss))
    
    print(f"Epoch {epoch+1} | Train Loss: {history['train_loss'][-1]:.4f} | Val Loss: {history['val_loss'][-1]:.4f}")

# Plot resulting training metrics
plt.plot(history['train_loss'], label='Train Loss')
plt.plot(history['val_loss'], label='Val Loss')
plt.title('Model Convergence and Loss Analysis')
plt.xlabel('Epochs')
plt.ylabel('Loss (BCE)')
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(frameon=True)
plt.savefig('sweet_spot.png')
model.export("amazons_model_v5_bke_hidden") 
print("Model saved at 'amazons_model_v5_bke_hidden'!")