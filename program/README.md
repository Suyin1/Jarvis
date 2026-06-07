# 契约驱动开发流水线 (Contract-Driven Development Pipeline)

**多阶段、多Worker的代码生成与维护流水线**，专为 ArkTS + QT 混合应用设计。将自然语言需求转化为可生成、可构建、可验证的代码，自带自愈闭环。

## 架构

```
需求 (自然语言)
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  Stage 1: 规约生成器 (SpecGenerator)                         │
│  将自然语言需求 → 结构化规约文档（契约）                       │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  Stage 2: 并行代码生成 (Worker 线程池)                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ QT C++   │  │ NAPI     │  │ ArkTS    │  │ 构建配置  │    │
│  │ Worker   │  │ Worker   │  │ Worker   │  │ Worker   │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  Stage 3: 集成与构建 (Builder)                                │
│  合并代码 → cmake 构建 → hvigorw 打包                         │
│  自动分类错误 → 生成修复任务                                   │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  Stage 4: 验证与自愈 (Verifier)                               │
│  运行测试 → 失败 → 自动修复 → 回溯 Stage 2                     │
│  最多 5 次迭代后升级到人工                                     │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│  ✓ 通过 → 归档 + 记忆更新                                    │
│  ✗ 失败 → 生成升级报告，等待人工介入                           │
└─────────────────────────────────────────────────────────────┘
```

## 快速开始

### 1. 安装依赖

```bash
pip install -r requirements.txt
```

### 2. 配置 LLM 后端

编辑 `config/pipeline.yaml`：

```yaml
llm:
  backend: "openai"    # openai | anthropic | azure | mock
  model: "gpt-4o"
```

通过环境变量设置 API 密钥：
```bash
export OPENAI_API_KEY="sk-..."
# 或
export ANTHROPIC_API_KEY="sk-ant-..."
```

### 3. 运行流水线

```bash
python main.py run "新增功能：QT侧文本输入，通过NAPI处理后返回ArkTS显示"
```

### 4. 查看结果

```bash
python main.py list                    # 列出最近运行记录
python main.py status <pipeline_id>    # 查看某次运行详情
python main.py memory list             # 查看积累的知识记忆
```

## 项目结构

```
├── main.py                     # CLI 入口
├── config/
│   ├── pipeline.yaml           # 流水线主配置（LLM、并行度、重试策略）
│   └── workers.yaml            # Worker 角色定义与规则
├── pipeline/                   # 核心引擎
│   ├── orchestrator.py         # 主调度器：编排所有阶段
│   ├── spec_generator.py       # Stage 1: 需求 → 结构化规约
│   ├── worker_pool.py          # Stage 2: 并行代码生成
│   ├── builder.py              # Stage 3: 集成 + 构建 + 错误分类
│   ├── verifier.py             # Stage 4: 验证 + 自愈 + 人工升级
│   ├── knowledge_base.py       # Stage 0: 知识库管理
│   ├── memory.py               # 记忆系统（相似检索 + few-shot）
│   ├── llm_client.py           # LLM 抽象层（多后端）
│   └── models.py               # 核心数据模型
├── prompts/                    # LLM System Prompt 模板
│   ├── spec_generator.md       # 规约架构师
│   ├── worker_cpp.md           # QT C++ 专家
│   ├── worker_napi.md          # NAPI 桥接专家
│   ├── worker_arkts.md         # ArkTS UI 专家
│   └── worker_config.md        # 构建配置专家
├── knowledge/                  # 知识库（Stage 0 资产）
│   ├── bridge_guide.md         # QT↔ArkTS NAPI桥接开发手册
│   └── templates/
│       └── spec_template.md    # 规约文档模板
├── scripts/
│   └── test.py                 # 自动化验证脚本
├── output/                     # Worker 生成的代码
│   ├── cpp/ / napi/ / ets/ / config/
├── workspace/                  # 集成工作区（构建在此进行）
├── specs/                      # 生成的规约文档
├── memory/                     # 持久化记忆（累积知识）
├── logs/                       # 运行日志
└── archive/                    # 成功运行的归档
```

## 核心设计原则

### 1. 契约驱动
每个阶段消费和产生标准化的文本文档。**规约文档（Specification）** 是所有阶段之间的"契约"，不需要共享 Agent 上下文。任何 Agent（甚至人）只需阅读上一阶段的产物即可接手工作。

### 2. 天然并行
Stage 2 同时启动多个 Worker 进程（QT C++、NAPI 桥接、ArkTS UI、构建配置）。每个 Worker 只操作自己模块的文件，互不干扰。

### 3. 脚本化，不靠幻觉
Stage 3（构建）和 Stage 4（验证）是**确定性脚本**，不使用 LLM 调用。在最关键的编译和测试环节杜绝幻觉。

### 4. 自愈闭环
流水线在 Stage 2→3→4 之间循环，最多 5 次（可配置）。自动按模块分类错误，将修复任务路由到正确的 Worker。

### 5. 进化记忆
每次成功运行都会向记忆库贡献知识。未来相似需求会自动获得历史 few-shot 示例，随时间推移持续提升生成质量。

### 6. LLM 无关
`BaseLLMClient` 抽象层支持 OpenAI、Anthropic、Azure、本地模型和 Mock 后端。切换后端无需修改任何业务代码。

## 配置说明

### LLM 后端

| 后端 | 配置值 | 必需环境变量 |
|------|--------|-------------|
| OpenAI | `openai` | `OPENAI_API_KEY` |
| Anthropic | `anthropic` | `ANTHROPIC_API_KEY` |
| Azure | `azure` | `AZURE_API_KEY`, `AZURE_API_BASE` |
| 本地（兼容OpenAI） | `local` | `OPENAI_API_BASE` |
| Mock（测试用） | `mock` | 无 |

### 流水线参数（`config/pipeline.yaml`）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `max_repair_iterations` | 5 | 自动修复最大循环次数 |
| `max_concurrent_workers` | 4 | Stage 2 并行 Worker 数 |
| `llm.temperature` | 0.3 | LLM 温度（越低越确定） |
| `verification.required_pass_rate` | 1.0 | 最低通过率（1.0 = 100%） |

## 扩展指南

### 添加新的 Worker 类型

1. 在 `config/workers.yaml` 中定义新 Worker
2. 在 `prompts/` 目录下创建对应的 System Prompt 模板
3. 在 `pipeline/models.py` 的 `WorkerType` 枚举中添加新类型
4. 在 `worker_pool.py` 的 `CodeWorker._default_prompt()` 中添加对应处理

### 添加新的 LLM 后端

1. 在 `pipeline/llm_client.py` 中创建新类继承 `BaseLLMClient`
2. 在 `LLMFactory._backends` 中注册
3. 在 `pipeline.yaml` 中设置 `llm.backend`

## 人工介入

当自动修复超过 `max_repair_iterations` 次后，流水线会：

1. 保存升级报告到 `reports/escalation_<id>.md`
2. 打印包含构建错误、测试结果和待修复任务的摘要
3. 等待人工给出方向性提示（如："NAPI 序列化应使用 rawdata 模式"）
4. 根据提供的指导继续执行

## 与业界方案对比

| 方案 | 本流水线的对应设计 |
|------|-------------------|
| **Devin** "计划-执行-验证" | 完整的 Stage 1→2→3→4 闭环，支持中断和回滚 |
| **MetaGPT** SOP 化 | 开发流程抽象为标准化操作流程，文档作为 Agent 间通信媒介 |
| **SWE-agent** 受限操作空间 | 每个 Worker 有明确的文件操作边界，不越界修改 |
| **RAG 模式** | 所有生成前检索知识库 + 历史记忆，保证一致性 |
| **AutoCodeRover** 结构化修复 | 基于错误日志分类归因，定向修复而非反复试错 |
