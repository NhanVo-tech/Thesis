#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""train_traj_model.py — train a Conv1D intent classifier on 2D UWB trajectories.

Input: CSV files produced by collect_traj.py with columns
    run_id, timestamp_ms, x, y, vx, vy, label
Labels: 0=approach, 1=seated, 2=leave, 3=passing

Outputs:
  * uwb_traj_model.h   — C header (alignas(16)) for TensorFlow Lite Micro
  * scaler mean/scale  — printed for traj_inference.cpp
  * accuracy + confusion matrix

Run in Colab (or any env with tensorflow + sklearn) after uploading the CSVs.
Mirrors the Stage-1 Conv1D architecture from LSTM_SCA.ipynb, but with 4 features
(x, y, vx, vy) and 4 classes.
"""

import os
import sys

import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.utils.class_weight import compute_class_weight

import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv1D, MaxPooling1D, Flatten, Dense, Dropout

import matplotlib.pyplot as plt
import seaborn as sns

# ---------------------------------------------------------------------------
# 1. Configuration
# ---------------------------------------------------------------------------
FILES = [
    "uwb_traj_data_label0.csv",
    "uwb_traj_data_label1.csv",
    "uwb_traj_data_label2.csv",
    "uwb_traj_data_label3.csv",
]
FEATURES = ["x", "y", "vx", "vy"]
CLASS_NAMES = ["approach", "seated", "leave", "passing"]

TIME_STEPS = 25
STEP_SIZE = 1
BATCH_SIZE = 32
EPOCHS = 100
TEST_SPLIT = 0.2
SEED = 42

# ---------------------------------------------------------------------------
# 2. Load + concat
# ---------------------------------------------------------------------------
print("Loading trajectory CSVs ...")
df_list = []
global_run_id = 0
for file in FILES:
    if not os.path.exists(file):
        print(f"  [skip] {file} not found")
        continue
    tmp = pd.read_csv(file)
    tmp = tmp.sort_values(by=["run_id", "timestamp_ms"])
    uniq = tmp["run_id"].unique()
    remap = {old: global_run_id + i for i, old in enumerate(uniq)}
    tmp["run_id"] = tmp["run_id"].map(remap)
    global_run_id += len(uniq)
    df_list.append(tmp)
    print(f"  {file}: {len(tmp)} rows, {len(uniq)} runs")

if not df_list:
    raise SystemExit("No data files found. Run collect_traj.py first.")

df = pd.concat(df_list, ignore_index=True)
print(f"Total: {df['run_id'].nunique()} runs, {len(df)} rows")
print("Label counts:\n", df["label"].value_counts().sort_index())

# ---------------------------------------------------------------------------
# 3. Train/test split by run (no leakage)
# ---------------------------------------------------------------------------
runs = df["run_id"].unique()
rng = np.random.RandomState(SEED)
rng.shuffle(runs)
split = int(len(runs) * (1 - TEST_SPLIT))
train_runs, test_runs = runs[:split], runs[split:]
train_df = df[df["run_id"].isin(train_runs)].copy()
test_df = df[df["run_id"].isin(test_runs)].copy()
print(f"Train runs: {len(train_runs)} | Test runs: {len(test_runs)}")

# ---------------------------------------------------------------------------
# 4. Normalize (fit on train, transform both)
# ---------------------------------------------------------------------------
scaler = StandardScaler()
train_df.loc[:, FEATURES] = scaler.fit_transform(train_df[FEATURES])
test_df.loc[:, FEATURES] = scaler.transform(test_df[FEATURES])

# ---------------------------------------------------------------------------
# 5. Sliding windows (per run)
# ---------------------------------------------------------------------------
def make_sequences(data, time_steps, step_size):
    X, y = [], []
    for _, group in data.groupby("run_id"):
        if len(group) < time_steps:
            continue
        vals = group[FEATURES].values
        labs = group["label"].values
        for i in range(0, len(group) - time_steps + 1, step_size):
            X.append(vals[i:i + time_steps])
            y.append(np.bincount(labs[i:i + time_steps]).argmax())
    return np.array(X), np.array(y)


X_train, y_train = make_sequences(train_df, TIME_STEPS, STEP_SIZE)
X_test, y_test = make_sequences(test_df, TIME_STEPS, STEP_SIZE)
print(f"X_train {X_train.shape}  X_test {X_test.shape}")

# ---------------------------------------------------------------------------
# 6. Model (Conv1D, mirrors Stage-1)
# ---------------------------------------------------------------------------
model = Sequential([
    Conv1D(16, kernel_size=3, activation="relu",
           input_shape=(TIME_STEPS, len(FEATURES))),
    MaxPooling1D(pool_size=2),
    Conv1D(32, kernel_size=3, activation="relu"),
    MaxPooling1D(pool_size=2),
    Flatten(),
    Dense(16, activation="relu"),
    Dropout(0.2),
    Dense(len(CLASS_NAMES), activation="softmax"),
])
model.compile(optimizer="adam", loss="sparse_categorical_crossentropy",
              metrics=["accuracy"])

class_weights_raw = compute_class_weight(
    "balanced", classes=np.unique(y_train), y=y_train)
class_weights = {i: class_weights_raw[i] for i in range(len(class_weights_raw))}

reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(
    monitor="val_loss", factor=0.2, patience=5, min_lr=0.0001, verbose=1)

print("\nTraining ...")
model.fit(X_train, y_train, epochs=EPOCHS, batch_size=BATCH_SIZE,
          validation_data=(X_test, y_test),
          class_weight=class_weights, callbacks=[reduce_lr], verbose=1)

# ---------------------------------------------------------------------------
# 7. Evaluate
# ---------------------------------------------------------------------------
loss, acc = model.evaluate(X_test, y_test, verbose=0)
print(f"\nTest accuracy: {acc * 100:.2f}%")

y_pred = np.argmax(model.predict(X_test), axis=1)
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(6, 5))
sns.heatmap(cm, annot=True, fmt="d", cmap="Blues",
            xticklabels=CLASS_NAMES, yticklabels=CLASS_NAMES)
plt.ylabel("true")
plt.xlabel("predicted")
plt.title(f"Confusion matrix — acc {acc * 100:.1f}%")
plt.show()
print(classification_report(y_test, y_pred, target_names=CLASS_NAMES))

# ---------------------------------------------------------------------------
# 8. Export TFLite C header
# ---------------------------------------------------------------------------
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS]
tflite_model = converter.convert()


def hex_to_c_array(hex_data, var_name):
    s = f"#ifndef {var_name.upper()}_H\n#define {var_name.upper()}_H\n\n"
    s += f"alignas(16) const unsigned char {var_name}[] = {{\n"
    for i in range(0, len(hex_data), 12):
        chunk = ", ".join(f"0x{b:02x}" for b in hex_data[i:i + 12])
        s += "  " + chunk + ",\n"
    s += f"}};\n\nconst unsigned int {var_name}_len = {len(hex_data)};\n\n"
    s += f"#endif // {var_name.upper()}_H\n"
    return s


with open("uwb_traj_model.h", "w") as f:
    f.write(hex_to_c_array(tflite_model, "uwb_traj_model"))
print("Wrote uwb_traj_model.h")

# ---------------------------------------------------------------------------
# 9. Scaler params for traj_inference.cpp
# ---------------------------------------------------------------------------
print("\n--- copy into traj_inference.cpp ---")
print("const float scaler_mean[NUM_FEATURES] = {"
      + ", ".join(f"{v:.6f}f" for v in scaler.mean_) + "};")
print("const float scaler_scale[NUM_FEATURES] = {"
      + ", ".join(f"{v:.6f}f" for v in scaler.scale_) + "};")
print("--------------------------------------")
