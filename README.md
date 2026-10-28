# 环境塑造理论：AI 表征转换评估沙盒

## 项目简介
本项目是一个基于 C++ 主控、真实大模型 API 驱动、Python 独立验证的 AI Agent 评估沙盒。
旨在探索“元规则（优先将代数转化为几何表征）”是否能在复杂的数学推理中，提升大模型的泛化与抗干扰能力。

## 目录结构
```text
├── tasks/              # 题库 (tasks.json)
├── answers/            # 模型回答存档 (按 A/B 组区分)
├── logs/               # 运行日志 (results_cpp.csv)
├── python/
│   ├── validator.py    # 独立验证器 (阅卷人)
│   └── stats.py        # 统计脚本
├── src/                # C++ 源码 (调度器、API请求、JSON清洗)
├── include/            # 依赖头文件 (nlohmann/json)
├── CMakeLists.txt      # C++ 构建配置
└── README.md

