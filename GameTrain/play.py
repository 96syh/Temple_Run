import torch
import time
import os
import numpy as np
from ParkourEnv import ParkourEnv

def play_demo(model, device, episodes, env=None):
    """
    通用 AI 演示函数
    :param model: 已经实例化的模型 (ActorNet 或 DQN)
    :param device: 计算设备 (cuda 或 cpu)
    :param episodes: 演示局数
    :param env: 可选，传入已有环境，若为 None 则新建 GUI 环境
    """
    print(f"\n" + "="*40)
    print("      AI 跑酷演示启动 (GUI 模式)")
    print("="*40)

    # 1. 初始化环境：如果未传入环境，则新建一个带窗口的
    own_env = False
    if env is None:
        env = ParkourEnv(headless=False)
        own_env = True
    
    model.eval() # 切换到评估模式，关闭 Dropout 等

    for episode in range(episodes):
        state, _ = env.reset()
        total_reward = 0
        done = False
        steps = 0
        
        print(f"开始演示第 {episode + 1} 局...")
        
        while not done:
            # 状态预处理并送入设备
            state_t = torch.FloatTensor(state).unsqueeze(0).to(device)
            
            # AI 推理：选取概率/Q值最大的动作
            with torch.no_grad():
                action_output = model(state_t)
                action = torch.argmax(action_output).item()
            
            # 执行动作并获取下一帧
            state, reward, terminated, truncated, _ = env.step(action)
            done = terminated or truncated
            total_reward += reward
            steps += 1
            
            # 帧率控制：由于 C++ 引擎太快，加入 20ms 延迟以便观察
            time.sleep(0.02)
            
            if steps > 2000: break

        print(f"  --> 结局统计 | 步数: {steps:4d} | 总奖励: {total_reward:6.1f}")
        time.sleep(0.5)

    # 如果是本函数创建的环境，则负责关闭
    if own_env:
        print("演示结束，正在释放 C++ 资源...")
        env.close()

def load_best_model(device):
    """
    按优先级加载最佳模型：A2C -> SARSA。
    返回：(model, model_name)；若都不存在则返回 (None, None)。
    """
    # 延迟导入，避免只想演示时触发其它脚本副作用
    from train_A2C import ActorNet
    from train_DQN import QNet

    a2c_path = "models/parkour_a2c_best.pth"
    sarsa_path = "models/parkour_sarsa_best.pth"

    if os.path.exists(a2c_path):
        model = ActorNet(5, 3).to(device)
        model.load_state_dict(torch.load(a2c_path, map_location=device))
        return model, "A2C"

    if os.path.exists(sarsa_path):
        model = QNet(5, 3).to(device)
        model.load_state_dict(torch.load(sarsa_path, map_location=device))
        return model, "SARSA"

    return None, None

# --- 独立运行块：方便你直接执行 python play.py 进行测试 ---
if __name__ == "__main__":
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    
    model, name = load_best_model(device)
    if model is None:
        print("错误：找不到模型文件 models/parkour_a2c_best.pth 或 models/parkour_sarsa_best.pth")
        raise SystemExit(1)

    print(f"已加载 {name} 最佳模型，开始演示...")
    play_demo(model, device, episodes=3)