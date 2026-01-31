# Changelog

## 2025-12-25

- Added smoother reward shaping in C++ environment:
  - Added step survival reward (`REWARD_STEP`)
  - Reduced death/hit penalty magnitudes for smoother curves
  - Exported `get_reward_step()` to Python via pybind11
- Updated Gymnasium wrapper:
  - Added `max_episode_steps` and proper `truncated` handling
  - Added clearer error when `GameEnv` extension cannot be imported
- Training scripts:
  - Replaced DQN training with **Deep SARSA** (on-policy TD target \(r + \gamma Q(s', a')\))
  - A2C: added advantage normalization, CSV logging, and separated model filename
- Added unified runner:
  - `GameTrain/run_all.py` to train SARSA/A2C and run GUI demo

