# EnvShape AI
# 环境塑造理论：AI 表征转换评估沙盒

## 项目简介
本项目是一个不依赖模型自述的 AI 评估沙盒。模型只输出答案，对错由独立的 Python 验证器判定。当前使用 Mock 模型跑通架构链路。

## 目录结构
```text
├── tasks/              # 题库 (tasks.json)
├── answers/            # 模型回答存档
├── logs/               # 运行日志 (results.csv)
├── prompts/            # 系统提示词 (group_a, group_b)
├── python/
│   ├── validator.py    # 独立验证器 (阅卷人)
│   ├── mock_model.py   # Mock 答题人
│   ├── model_interface.py # 接口层
│   ├── run_experiment.py  # 调度器
│   └── stats.py        # 统计脚本
├── src/                # C++ 源码
├── CMakeLists.txt      # C++ 构建配置
└── README.md
