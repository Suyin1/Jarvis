# AI Agent 维护指南

> 本项目专为 AI Agent 维护而设计。如果你是 AI Agent，请先阅读本文档。

---

## 项目速览

| 项目 | 内容 |
|------|------|
| 项目名称 | 资料解决方案系统 (Doc Solution System) |
| 技术栈 | Python 3.11+ / Vale / Jinja2 |
| 当前阶段 | Phase 1: CLI 工具集 |
| 交付形态 | CLI 命令行工具 |
| 核心能力 | 质量检查 / 内容生成 / 知识库构建 |

## 目录结构

```
03-系统项目/
├── AGENTS.md              ← 你在这里，AI Agent 维护指南
├── README.md              ← 项目总览（给你和人类看）
├── ROADMAP.md             ← 开发路线图（当前和未来计划）
├── DEVELOPMENT_LOG.md     ← 开发进度记录（每次修改后更新）
├── EVOLUTION_STRATEGY.md  ← 演进策略（请先阅读了解架构方向）
│
├── knowledge/             ← 知识库 (客户专属规则/模板/术语)
├── engine/                ← 核心引擎 (Python 代码)
├── tools/                 ← CLI 工具入口
├── mcp/                   ← MCP Server (Phase 2, 当前为空)
├── schemas/               ← JSON Schema 定义
├── tests/                 ← 测试
└── examples/              ← 示例配置
```

## AI Agent 工作流

当你收到维护任务时，请按以下步骤操作：

### 1. 理解上下文

```yaml
先读:
  - AGENTS.md       # 本文件 - 维护指南
  - EVOLUTION_STRATEGY.md  # 架构演进方向
  - ROADMAP.md      # 当前开发计划
  - DEVELOPMENT_LOG.md  # 历史变更记录
```

### 2. 执行任务

```yaml
修改代码:
  1. 修改文件
  2. 运行测试: pytest tests/
  3. 更新 DEVELOPMENT_LOG.md
  4. 更新 ROADMAP.md (如适用)

新增功能:
  1. 确认功能的位置 (engine/ 还是 tools/ 还是 knowledge/)
  2. 实现功能
  3. 添加测试
  4. 运行测试确保通过
  5. 更新文档 (AGENTS.md / README.md 如适用)
  6. 更新 DEVELOPMENT_LOG.md
  7. 更新 ROADMAP.md (标记完成/调整计划)
```

### 3. 文档维护规范

```yaml
DEVELOPMENT_LOG.md 更新格式:
  ### YYYY-MM-DD - 本次工作标题
  
  | 任务 | 状态 | 说明 |
  |------|------|------|
  | <做了什么> | ✅/⏳/❌ | <简要说明> |
  
  #### 变更详情
  - <文件路径>: <变更内容>
  - ...

ROADMAP.md 更新:
  - 完成任务标记为 ✅
  - 新增任务追加到对应阶段
  - 计划不变更则不动
```

### 4. 重要约束

| 约束 | 说明 |
|------|------|
| 不引入 LLM 依赖 | 解决方案本身不含任何 LLM 或 Agent 产品 |
| 本地优先 | 新增功能不得依赖网络服务 |
| 向下兼容 | Phase 1 CLI 功能不能因后续升级而失效 |
| AI 可读 | 配置文件使用 YAML/JSON，代码有类型标注 |
| 可测试 | 新功能必须有对应的测试 |

### 5. 快速参考

```bash
# 安装项目
pip install -e .

# 运行所有测试
pytest tests/

# 运行特定测试
pytest tests/test_engine/

# 使用 CLI 工具 (安装后)
doc-solution check --target ./docs/
doc-solution generate --template api-ref
doc-solution build-kb --input ./customer-inputs/

# 检查 Python 代码风格 (不强制，但推荐)
ruff check .
```

---

如果你在读这个文件但没有执行维护任务，请从 README.md 开始了解项目。
