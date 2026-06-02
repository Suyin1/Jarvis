# 资料解决方案系统 — 产品使用指南

> 面向客户团队的说明书：如何使用本系统提升资料开发效率

---

## 一、这是什么？

这是一套**资料开发全链路解决方案**，覆盖文档质量检查、内容生成、知识库构建三大核心能力。您的 AI Agent 可以调用本系统完成自动化的资料开发任务。

**核心价值：**
- 减少重复劳动 —— AI 主导开发，人力审核
- 保证质量一致 —— 自动检查格式/风格/结构
- 适配您的风格 —— 根据您的源文档自动构建知识库

## 二、快速开始

### 2.1 环境准备

本系统依赖以下工具（均已预装）：

| 工具 | 用途 | 备注 |
|------|------|------|
| Python 3.6+ | 运行环境 | ✅ 已就绪 |
| click / pyyaml / jinja2 | CLI + YAML + 模板引擎 | ✅ 已安装 |
| Vale (可选) | 增强文档检查 | 如需安装: `npm install -g @errata-ai/vale` |

### 2.2 验证安装

```bash
# 查看版本
python -m tools.cli --help

# 运行测试 (确保一切正常)
python -m pytest tests/ -v

# 检查示例文档
python -m tools.cli check --target examples/sample-docs/sample-guide.md
```

## 三、使用场景

### 场景一：接入您的团队知识

```bash
# 将您的源文档、模板、标准等存到一个目录，然后：
python -m tools.cli build-kb \
  --input ./your-docs/ \
  --name "您的团队名称"
```

系统会自动分析您的文档风格、提取术语、注册模板。

### 场景二：生成新文档

```bash
# 基于 API 参考模板生成文档
python -m tools.cli generate \
  --template api-ref \
  --params '{"api_name": "startAbility", "declaration": "..."}'
```

### 场景三：质量检查

```bash
# 全量检查您的文档
python -m tools.cli check --target ./docs/ --check-type all

# 只看结构问题
python -m tools.cli check --target ./docs/ --check-type structure

# 输出 JSON 报告
python -m tools.cli check --target ./docs/ --output json --save-report report.json
```

## 四、与 AI Agent 配合使用

您的 AI Agent 可以通过以下方式使用本系统：

### 方式一：直接调用 CLI

```bash
# Agent 执行 shell 命令
doc-solution check --target ./docs/
doc-solution generate --template api-ref --params '{...}'
doc-solution build-kb --input ./customer-inputs/ --name "客户名"
```

### 方式二：读取项目文档

AI Agent 应先阅读以下文档了解系统：

| 文档 | 用途 |
|------|------|
| `USAGE.md` | AI Agent 使用指南，含完整命令说明和最佳实践 |
| `AGENTS.md` | AI Agent 维护指南，含开发规范 |
| `ROADMAP.md` | 开发路线图 |
| `DEVELOPMENT_LOG.md` | 开发历史记录 |

## 五、项目结构说明

```
03-系统项目/
├── USAGE.md            # AI Agent 使用指南
├── PRODUCT_GUIDE.md    # 本文件，客户使用说明
├── TESTING_GUIDE.md    # 回归测试指南
├── AGENTS.md           # AI 维护指南
├── README.md           # 项目总览
├── ROADMAP.md          # 开发路线图
├── DEVELOPMENT_LOG.md  # 开发进度记录
│
├── engine/             # 核心引擎 (Python)
├── tools/              # CLI 工具入口
├── mcp/                # MCP Server (预留)
├── knowledge/          # 知识库配置
├── schemas/            # JSON Schema 定义
├── tests/              # 测试
└── examples/           # 示例
```

## 六、常见问题

### Q: 需要联网吗？

**不需要。** 所有核心功能完全离线运行。

### Q: 依赖什么外部工具？

核心功能只依赖 Python 标准库 + 三个包（click、pyyaml、jinja2，均已安装）。Vale 是可选项，用于增强文档检查能力，不装也不影响基本功能。

### Q: 如何自定义规则？

有两种方式：
1. **知识库构建**：`build-kb` 命令会自动从您的源文档中提取风格特征
2. **手动配置**：直接编辑 `knowledge/rules/custom/` 下的 YAML 文件

### Q: 如何更新知识库？

```bash
# 增量更新：重新运行 build-kb
doc-solution build-kb --input ./updated-docs/ --name "客户名" --force
```

### Q: 出了问题怎么办？

1. 运行 `python -m pytest tests/ -v` 确认系统本身正常
2. 查看 `DEVELOPMENT_LOG.md` 了解最近变更
3. 检查知识库配置 `knowledge/config.yaml`
