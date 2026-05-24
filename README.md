# Temple_Run

基于 C++ 游戏环境和 Python 强化学习脚本的跑酷类训练实验。项目包含一个 `GameEnv` 环境扩展，以及使用 Gymnasium 接口包装后的 A2C/DQN 训练脚本。

## 项目组成

```text
.
├── GameEnv/             # C++ 游戏环境工程
├── GameTrain/           # Python 训练脚本
│   ├── ParkourEnv.py    # Gymnasium 环境封装
│   ├── train_A2C.py     # A2C 训练入口
│   ├── train_DQN.py     # DQN 训练入口
│   ├── play.py          # 模型运行/演示入口
│   ├── run_all.py       # 批量运行入口
│   ├── GameEnv.pyd      # Windows Python 扩展
│   └── requirements.txt
└── readme
```

## 环境依赖

Python 训练侧依赖：

```text
torch>=2.0.0
numpy>=1.24.0
gymnasium>=0.28.0
```

安装：

```bash
cd GameTrain
python -m pip install -r requirements.txt
```

## 运行说明

`ParkourEnv.py` 会延迟导入 `GameEnv` 扩展模块，并将游戏环境封装为 Gymnasium 环境：

- action space: `Discrete(3)`
- observation space: 5 维浮点向量
- 支持 episode step 上限
- 返回 score 等基础信息

训练入口：

```bash
python train_DQN.py
python train_A2C.py
```

## 平台注意事项

仓库中包含的是 Windows `.pyd` 扩展。macOS 或 Linux 不能直接加载该文件，需要在对应平台重新编译 `GameEnv` 扩展，或在 Windows 环境中运行训练脚本。

## 当前状态

这是一个强化学习实验项目，重点在环境封装和训练脚本。继续维护时建议优先补充：

- 环境编译步骤
- 训练参数说明
- 模型输出目录说明
- 一条最小可复现实验命令

