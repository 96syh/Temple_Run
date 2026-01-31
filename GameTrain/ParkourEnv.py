import gymnasium as gym
from gymnasium import spaces
import numpy as np

def _import_gameenv():
    """
    延迟导入 C++ 扩展模块，方便给出更友好的报错。
    注意：Windows 通常是 .pyd；macOS/Linux 通常是 .so。
    """
    try:
        import GameEnv  # type: ignore
        return GameEnv
    except Exception as e:
        raise RuntimeError(
            "无法导入 C++ 扩展模块 `GameEnv`。\n"
            "可能原因：\n"
            "1) 你当前是 macOS/Linux，但目录里是 Windows 的 `GameEnv.pyd`（不能加载）。\n"
            "2) `GameEnv` 没有放在 Python 可搜索路径（需要与脚本同目录或已安装）。\n"
            "解决建议：\n"
            "- 在 Windows 上运行 `GameTrain`（使用现成的 `GameEnv.pyd`），或\n"
            "- 在你的系统上重新编译生成对应平台的扩展（macOS/Linux 需要 `.so`）。\n"
            f"原始错误：{repr(e)}"
        )

class ParkourEnv(gym.Env):
    def __init__(self, headless=True, max_episode_steps=2000):
        super(ParkourEnv, self).__init__()
        GameEnv = _import_gameenv()
        self.game = GameEnv.GameEnv(headless)
        self.action_space = spaces.Discrete(3)
        self.observation_space = spaces.Box(
            low=0.0, high=1.0, shape=(5,), dtype=np.float32
        )
        # 训练时建议加一个时间上限，区分 terminated / truncated，有利于算法 bootstrapping 正确性
        self.max_episode_steps = int(max_episode_steps) if max_episode_steps is not None else None
        self._step_count = 0

    # --- 必须添加这些转发接口，否则 train.py 会报 AttributeError ---
    def get_reward_pass(self):
        return self.game.get_reward_pass()

    def get_reward_death(self):
        return self.game.get_reward_death()

    def get_reward_hit(self): 
        return self.game.get_reward_hit() 

    def get_reward_step(self):
        return self.game.get_reward_step()
    
    def get_damage_taken(self):
        return self.game.get_damage_taken()

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.game.reset()
        self._step_count = 0
        obs = self.game.get_obs()
        return np.array(obs, dtype=np.float32), {}

    def step(self, action):
        result = self.game.step(action)
        observation = np.array(result.observation, dtype=np.float32)
        reward = float(result.reward)
        terminated = bool(result.done)
        self._step_count += 1
        truncated = False
        if self.max_episode_steps is not None and self._step_count >= self.max_episode_steps and not terminated:
            truncated = True
        info = {"score": result.score}
        return observation, reward, terminated, truncated, info