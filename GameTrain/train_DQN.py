import random
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
import time
import os
from ParkourEnv import ParkourEnv
from play import play_demo  # 导入通用演示函数
import csv

# --- 硬件检测 ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"系统自检：PyTorch 正在使用 -> {device}")

# --- 默认超参数（也支持从外部主入口传参覆盖）---
DEFAULT_EPISODES = 200                 # 建议至少 200 轮，reward 曲线会更平滑
DEFAULT_MAX_STEPS_PER_EPISODE = 2000   # 与 ParkourEnv(max_episode_steps) 对齐
DEFAULT_GAMMA = 0.99
DEFAULT_EPSILON_START = 1.0
DEFAULT_EPSILON_MIN = 0.05
DEFAULT_EPSILON_DECAY = 0.995          # 更慢衰减，SARSA 更依赖 on-policy 探索
DEFAULT_LEARNING_RATE = 1e-3

class QNet(nn.Module):
    def __init__(self, state_size, action_size):
        super(QNet, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_size, 128),
            nn.ReLU(),
            nn.Linear(128, 128),
            nn.ReLU(),
            nn.Linear(128, action_size)
        )
    def forward(self, x):
        return self.fc(x)

def _select_action(qnet: QNet, state_t: torch.Tensor, action_size: int, epsilon: float) -> int:
    """Epsilon-Greedy 选动作（SARSA 需要 next_action 也走同一策略，确保 on-policy）。"""
    if random.random() <= epsilon:
        return random.randrange(action_size)
    with torch.no_grad():
        return torch.argmax(qnet(state_t)).item()

def _append_csv_row(path: str, row: dict):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    file_exists = os.path.exists(path)
    with open(path, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(row.keys()))
        if not file_exists:
            writer.writeheader()
        writer.writerow(row)

def train_parkour_sarsa(
    episodes: int = DEFAULT_EPISODES,
    max_steps_per_episode: int = DEFAULT_MAX_STEPS_PER_EPISODE,
    gamma: float = DEFAULT_GAMMA,
    epsilon_start: float = DEFAULT_EPSILON_START,
    epsilon_min: float = DEFAULT_EPSILON_MIN,
    epsilon_decay: float = DEFAULT_EPSILON_DECAY,
    learning_rate: float = DEFAULT_LEARNING_RATE,
    seed: int | None = None,
    demo_episodes: int = 3,
):
    print("Step 1: 正在初始化 C++ 物理引擎...")
    env = ParkourEnv(headless=True, max_episode_steps=max_steps_per_episode)

    if seed is not None:
        random.seed(seed)
        np.random.seed(seed)
        torch.manual_seed(seed)
    
    # 打印完整的配置信息
    print("\n" + "="*40)
    print("      RL 环境奖励参数配置 (C++ 导出)")
    print("-" * 40)
    try:
        print(f"  [+] 通过障碍物奖励:  +{env.get_reward_pass()}")
        print(f"  [!] 碰到障碍物奖励:   {env.get_reward_hit()}")
        print(f"  [†] 死亡奖励/惩罚:    {env.get_reward_death()}")
        print(f"  [+] 生存微奖励/步:   +{env.get_reward_step()}")
        print(f"  [-] 碰撞单次扣血量:  -{env.get_damage_taken()}")
    except AttributeError:
        print("  [!] 警告: 参数导出失败")
    print("=" * 40 + "\n")

    state_size = env.observation_space.shape[0]
    action_size = env.action_space.n
    qnet = QNet(state_size, action_size).to(device)
    optimizer = optim.Adam(qnet.parameters(), lr=learning_rate)
    
    if not os.path.exists('models'): os.makedirs('models')
    if not os.path.exists('runs'): os.makedirs('runs')
    
    epsilon = float(epsilon_start)
    best_reward = -float('inf') # 追踪历史最高奖励
    rewards_window = []
    csv_path = os.path.join("runs", "sarsa_rewards.csv")
    
    print(f"Step 2: 进入训练循环... (算法: Deep SARSA)")

    for episode in range(int(episodes)):
        state, _ = env.reset()
        state_t = torch.as_tensor(state, dtype=torch.float32, device=device).unsqueeze(0)
        total_reward = 0
        step_count = 0

        # SARSA：先选一个动作 a（on-policy）
        action = _select_action(qnet, state_t, action_size, epsilon)

        done = False
        while not done:
            # 执行一步
            next_state, reward, terminated, truncated, _ = env.step(action)
            done = terminated or truncated

            step_count += 1
            total_reward += reward

            next_state_t = torch.as_tensor(next_state, dtype=torch.float32, device=device).unsqueeze(0)

            # SARSA：用同一 epsilon-greedy 策略选 next_action
            if not done:
                next_action = _select_action(qnet, next_state_t, action_size, epsilon)

                # TD target: r + gamma * Q(s', a')
                with torch.no_grad():
                    target_q = reward + gamma * qnet(next_state_t)[0, next_action].item()
            else:
                next_action = None
                target_q = reward

            # current Q(s,a)
            current_q = qnet(state_t)[0, action]
            loss = (current_q - torch.tensor(target_q, device=device, dtype=torch.float32)).pow(2)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(qnet.parameters(), max_norm=10.0)
            optimizer.step()

            # 推进
            state_t = next_state_t
            if next_action is not None:
                action = next_action

        epsilon = max(float(epsilon_min), epsilon * float(epsilon_decay))
        rewards_window.append(total_reward)
        if len(rewards_window) > 50:
            rewards_window.pop(0)
        avg50 = float(np.mean(rewards_window))

        print(f"[SARSA] Episode {episode+1:3d} | Steps: {step_count:4d} | Total Reward: {total_reward:7.2f} | Avg50: {avg50:7.2f} | Epsilon: {epsilon:.3f}")
        _append_csv_row(csv_path, {
            "episode": episode + 1,
            "steps": step_count,
            "total_reward": float(total_reward),
            "avg50_reward": avg50,
            "epsilon": float(epsilon),
        })

        # --- 最佳模型保存逻辑 ---
        if total_reward > best_reward:
            best_reward = total_reward
            torch.save(qnet.state_dict(), "models/parkour_sarsa_best.pth")
            print(f"  --> 已保存 SARSA 历史最好模型 (Reward: {total_reward:.2f})")

    # --- 训练结束，自动开始演示 ---
    print("\n训练全部完成！正在加载 SARSA 最佳模型进行演示...")
    
    # 创建演示用的模型并加载最佳权重
    best_model = QNet(state_size, action_size).to(device)
    best_path = "models/parkour_sarsa_best.pth"
    if os.path.exists(best_path):
        best_model.load_state_dict(torch.load(best_path, map_location=device))
    
    # 调用 play.py 中的函数
    play_demo(best_model, device, episodes=int(demo_episodes))

if __name__ == "__main__":
    train_parkour_sarsa()