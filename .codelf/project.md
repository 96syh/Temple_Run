# Project Overview

## Name
Parkour RL (天天酷跑/跑酷游戏 + 强化学习训练)

## What this repo does
This project contains a C++ parkour game environment exported to Python via `pybind11`, plus Python training scripts to train an agent using:
- **Deep SARSA** (modified from the original DQN script)
- **A2C**

The goal is to let the AI survive longer and produce smoother, nicer reward curves.

## Key directories
- `GameEnv/`: C++ EasyX-based game + pybind11 bindings (exports module `GameEnv`)
- `GameTrain/`: Python Gymnasium wrapper + training/evaluation scripts

## Entry points
- `GameTrain/run_all.py`: unified entry to train SARSA/A2C and run GUI demo
- `GameTrain/train_DQN.py`: Deep SARSA training (function `train_parkour_sarsa`)
- `GameTrain/train_A2C.py`: A2C training (function `train_parkour_a2c`)
- `GameTrain/play.py`: GUI demo (function `play_demo`, loader `load_best_model`)

## Outputs
- Models:
  - `GameTrain/models/parkour_sarsa_best.pth`
  - `GameTrain/models/parkour_a2c_best.pth`
- Reward logs:
  - `GameTrain/runs/sarsa_rewards.csv`
  - `GameTrain/runs/a2c_rewards.csv`

## Notes
- Platform-specific extension:
  - Windows typically uses `GameEnv.pyd`
  - macOS/Linux typically needs a `.so` build

