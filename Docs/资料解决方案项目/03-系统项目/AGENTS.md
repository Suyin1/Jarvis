---
audience: ai-agent
priority: high
purpose: AI Agent maintenance guide for the Doc Solution System
category: reference
last-updated: 2026-06-03
---

# AI Agent 维护指南

> 本项目专为 AI Agent 维护而设计。如果你是 AI Agent，请先阅读本文档。

---

## 项目速览

| 项目 | 内容 |
|------|------|
| 项目名称 | 资料解决方案系统 (Doc Solution System) |
| 技术栈 | Python 3.6+ / Vale / Jinja2 |
| 当前阶段 | v0.2 (CLI + MCP 完整) |
| 交付形态 | CLI 命令行工具 + MCP Server |
| 核心能力 | 质量检查 / 内容生成 / 知识库构建 |
| 测试 | 39 个通过 |

## 文档体系

见 `README.md` 的文档分类索引。关键词：

| 读者 | 入口文档 |
|------|----------|
| AI Agent（你） | `USAGE.md` + `docs/*.md` |
| 客户 | `PRODUCT_GUIDE.md` + `docs/customer/SECURITY.md` |
| 两者 | `README.md` / `ROADMAP.md` |

## AI Agent 工作流

### 1. 理解上下文

```yaml
先读:
  - README.md           # 入口 + 文档索引
  - AGENTS.md           # 本文件 - 维护指南
  - USAGE.md            # CLI/MCP 调用参考
  - docs/kb-construction-guide.md  # 知识库构建方法论(Vale规则/术语/清单/测试标准)
  - ../02-设计方案/演进策略与架构决策记录.md  # 架构演进方向
  - ROADMAP.md          # 当前开发计划
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
  1. 确认功能的位置 (engine/ / tools/ / knowledge/)
  2. 实现功能
  3. 添加测试
  4. 运行测试确保通过
  5. 更新相关文档 (docs/*.md, docs/kb-construction-guide.md)
  6. 更新 DEVELOPMENT_LOG.md
  7. 更新 ROADMAP.md
```

### 3. 文档更新规范

```yaml
所有文档需 YAML frontmatter:
  ---
  audience: ai-agent|customer|both
  category: guide|reference|architecture|security
  priority: high|medium|low
  last-updated: YYYY-MM-DD
  ---

DEVELOPMENT_LOG.md 更新格式:
  ### YYYY-MM-DD - 标题
  | 任务 | 状态 | 说明 |
  |------|------|------|
  | ...  | ✅   | ...  |
  #### 变更详情
  - <文件路径>: <变更内容>

ROADMAP.md 更新:
  - 完成任务标记为 ✅
  - 新增任务追加到对应阶段
```

### 4. 重要约束

| 约束 | 说明 |
|------|------|
| 不引入 LLM 依赖 | 解决方案本身不含任何 LLM 或 Agent 产品 |
| 本地优先 | 新增功能不得依赖网络服务 |
| 向下兼容 | CLI 功能不能因后续升级而失效 |
| AI 可读 | 配置文件使用 YAML/JSON，代码有类型标注 |
| 可测试 | 新功能必须有对应的测试 |
| 零网络 | 所有组件无任何网络请求 |

### 5. 快速参考

```bash
# 安装
pip install -e .

# 运行所有测试
pytest tests/ -v

# 运行特定测试
pytest tests/test_engine/

# CLI
doc-solution check --target ./docs/
doc-solution generate --template api-ref
doc-solution build-kb --input ./customer-inputs/

# MCP Server (stdio)
python -m mcp.server
```

---

如果不在执行维护任务，从 `README.md` 开始了解项目。
