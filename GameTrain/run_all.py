import argparse

from train_DQN import train_parkour_sarsa
from train_A2C import train_parkour_a2c
from play import play_demo, load_best_model


def _print_header():
    print("\n" + "=" * 60)
    print("天天酷跑强化学习总入口 (SARSA + A2C)")
    print("=" * 60)
    print("说明：训练默认使用 headless=True（不弹窗），演示阶段会打开 GUI 窗口。")
    print("如果提示无法导入 `GameEnv`，请优先检查平台是否匹配（Windows .pyd / macOS/Linux .so）。")
    print("=" * 60 + "\n")


def main():
    parser = argparse.ArgumentParser(description="Run SARSA/A2C training and demos for ParkourEnv.")
    parser.add_argument("--mode", type=str, default="all",
                        choices=["all", "sarsa", "a2c", "demo"],
                        help="all: 训练 SARSA + A2C 并演示；sarsa/a2c: 只训练其中一个；demo: 只演示已保存模型。")
    parser.add_argument("--seed", type=int, default=0, help="随机种子（0 表示不固定）。")
    parser.add_argument("--max-steps", type=int, default=2000, help="每局最大步数（会作为 truncated）。")
    parser.add_argument("--episodes", type=int, default=200, help="训练轮数（SARSA/A2C 共用）。")
    parser.add_argument("--demo-episodes", type=int, default=3, help="演示局数。")
    args = parser.parse_args()

    _print_header()

    seed = None if args.seed == 0 else int(args.seed)

    if args.mode in ("all", "sarsa"):
        print(">>> 开始训练：Deep SARSA")
        train_parkour_sarsa(
            episodes=args.episodes,
            max_steps_per_episode=args.max_steps,
            seed=seed,
            demo_episodes=0,  # 总入口里统一演示
        )
        print(">>> Deep SARSA 训练结束\n")

    if args.mode in ("all", "a2c"):
        print(">>> 开始训练：A2C")
        train_parkour_a2c(
            episodes=args.episodes,
            max_steps=args.max_steps,
            seed=seed,
            demo_episodes=0,  # 总入口里统一演示
        )
        print(">>> A2C 训练结束\n")

    if args.mode in ("all", "demo"):
        print(">>> 开始演示（GUI 模式）")
        import torch
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        model, name = load_best_model(device)
        if model is None:
            print("错误：找不到模型文件 models/parkour_a2c_best.pth 或 models/parkour_sarsa_best.pth")
            raise SystemExit(1)
        print(f"已加载 {name} 最佳模型，开始演示...")
        play_demo(model, device, episodes=int(args.demo_episodes))

    print("\n全部流程结束。训练曲线数据已输出到 runs/ 目录：")
    print(" - runs/sarsa_rewards.csv")
    print(" - runs/a2c_rewards.csv")


if __name__ == "__main__":
    main()


