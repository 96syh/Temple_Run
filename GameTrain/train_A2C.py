import gymnasium as gym
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.distributions import Categorical
import os
from ParkourEnv import ParkourEnv
from play import play_demo  # 导入刚才定义的演示函数
import csv

# --- 硬件检测 ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"系统自检：PyTorch 正在使用 -> {device}")

# --- A2C 默认超参数（也支持从外部主入口传参覆盖）---
DEFAULT_EPISODES = 200               # 建议至少 200 轮，reward 曲线更平滑
DEFAULT_GAMMA = 0.99                 # 奖励折扣率
DEFAULT_ACTOR_LR = 1e-3              # Actor 学习率
DEFAULT_CRITIC_LR = 5e-3             # Critic 学习率
DEFAULT_ENTROPY_COEF = 0.01          # 熵权重，防止过早收敛
DEFAULT_UPDATE_STEPS = 50            # 分段更新步数，防止显存溢出
DEFAULT_MAX_STEPS = 2000             # 单局步数上限

# ==========================================
# 网络架构 (适配离散动作空间)
# ==========================================

class ActorNet(nn.Module):
    def __init__(self, state_dim, action_dim):
        super(ActorNet, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_dim, 128), nn.ReLU(),
            nn.Linear(128, 128), nn.ReLU(),
            nn.Linear(128, action_dim),
            nn.Softmax(dim=-1) # 输出动作概率
        )
    def forward(self, x): return self.fc(x)

class CriticNet(nn.Module):
    def __init__(self, state_dim):
        super(CriticNet, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_dim, 128), nn.ReLU(),
            nn.Linear(128, 128), nn.ReLU(),
            nn.Linear(128, 1) # 输出状态价值 V(s)
        )
    def forward(self, x): return self.fc(x)

# ==========================================
# 训练主函数
# ==========================================

def _append_csv_row(path: str, row: dict):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    file_exists = os.path.exists(path)
    with open(path, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(row.keys()))
        if not file_exists:
            writer.writeheader()
        writer.writerow(row)

def train_parkour_a2c(
    episodes: int = DEFAULT_EPISODES,
    gamma: float = DEFAULT_GAMMA,
    actor_lr: float = DEFAULT_ACTOR_LR,
    critic_lr: float = DEFAULT_CRITIC_LR,
    entropy_coef: float = DEFAULT_ENTROPY_COEF,
    update_steps: int = DEFAULT_UPDATE_STEPS,
    max_steps: int = DEFAULT_MAX_STEPS,
    seed: int | None = None,
    demo_episodes: int = 3,
):
    # 1. 初始化
    env = ParkourEnv(headless=True, max_episode_steps=max_steps)
    state_dim = env.observation_space.shape[0]
    action_dim = env.action_space.n

    actor = ActorNet(state_dim, action_dim).to(device)
    critic = CriticNet(state_dim).to(device)
    actor_opt = optim.Adam(actor.parameters(), lr=actor_lr)
    critic_opt = optim.Adam(critic.parameters(), lr=critic_lr)

    if not os.path.exists('models'): os.makedirs('models')
    best_reward = -float('inf')

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

    csv_path = os.path.join("runs", "a2c_rewards.csv")
    rewards_window = []

    if seed is not None:
        np.random.seed(seed)
        torch.manual_seed(seed)

    for episode in range(int(episodes)):
        state, _ = env.reset()
        total_reward = 0
        done = False
        episode_steps = 0

        while not done and episode_steps < int(max_steps):
            # 数据暂存区（用于 On-Policy 分段更新）
            log_probs, values, rewards, masks, entropies = [], [], [], [], []
            
            # --- 采集阶段 ---
            for _ in range(int(update_steps)):
                state_t = torch.FloatTensor(state).unsqueeze(0).to(device)
                probs = actor(state_t)
                dist = Categorical(probs)
                action = dist.sample()
                value = critic(state_t)

                next_state, reward, terminated, truncated, _ = env.step(action.item())
                done = terminated or truncated
                
                log_probs.append(dist.log_prob(action))
                values.append(value)
                rewards.append(torch.FloatTensor([reward]).to(device))
                masks.append(torch.FloatTensor([1 - done]).to(device))
                entropies.append(dist.entropy())

                state = next_state
                total_reward += reward
                episode_steps += 1
                if done: break

            # --- 计算阶段 ---
            next_value = critic(torch.FloatTensor(state).unsqueeze(0).to(device))
            returns = []
            R = next_value.detach()
            for r, m in zip(reversed(rewards), reversed(masks)):
                R = r + gamma * R * m
                returns.insert(0, R)

            # --- 核心修复：展平张量，消除 MSELoss 广播警告 ---
            returns = torch.cat(returns).detach().view(-1)      
            log_probs_t = torch.cat(log_probs).view(-1)
            values_t = torch.cat(values).view(-1)
            entropies_t = torch.stack(entropies).view(-1)

            advantage = returns - values_t # 计算优势
            # 优势归一化：更稳、更不容易抖（reward 曲线更好看）
            advantage = (advantage - advantage.mean()) / (advantage.std() + 1e-8)

            # --- 更新阶段 ---
            actor_loss = -(log_probs_t * advantage.detach()).mean() - entropy_coef * entropies_t.mean()
            critic_loss = F.mse_loss(values_t, returns) # 现在 shapes 匹配

            actor_opt.zero_grad(); actor_loss.backward(); actor_opt.step()
            critic_opt.zero_grad(); critic_loss.backward(); critic_opt.step()

        rewards_window.append(total_reward)
        if len(rewards_window) > 50:
            rewards_window.pop(0)
        avg50 = float(np.mean(rewards_window))

        print(f"[A2C ] Episode {episode+1:3d} | Steps: {episode_steps:4d} | Total Reward: {total_reward:7.2f} | Avg50: {avg50:7.2f}")
        _append_csv_row(csv_path, {
            "episode": episode + 1,
            "steps": episode_steps,
            "total_reward": float(total_reward),
            "avg50_reward": avg50,
        })
        
        # 保存表现最好的模型
        if total_reward > best_reward:
            best_reward = total_reward
            torch.save(actor.state_dict(), "models/parkour_a2c_best.pth")
            print("  --> 已保存 A2C 历史最好模型")

        if (episode + 1) % 50 == 0: torch.cuda.empty_cache()

# --- 训练结束逻辑 ---
    print("\n训练全部结束。")

    # 核心修复：演示前必须加载保存的最佳权重
    print("正在加载历史最佳模型进行演示...")
    best_model_path = "models/parkour_a2c_best.pth"
    
    if os.path.exists(best_model_path):
        # 将磁盘上的最佳参数覆盖到当前的 actor 对象中
        actor.load_state_dict(torch.load(best_model_path, map_location=device))
        print(f"成功加载最佳模型权重！(Score: {best_reward:.1f})")
    else:
        print("警告：未找到最佳模型文件，将使用最终模型进行演示。")

    # 现在调用的 actor 已经是“巅峰状态”的版本了
    play_demo(actor, device, episodes=int(demo_episodes))

if __name__ == "__main__":
    train_parkour_a2c()