# Doc Solution — 资料开发全链路解决方案

> CLI 工具集 · 质量检查 · 内容生成 · 知识库构建 · 问题解决 · 维护扫描

---

## 快速开始

```bash
# 安装
pip install -e .

# 检查文档质量
doc-solution check --target ./docs/

# 生成文档内容
doc-solution generate --template api-ref --params '{"api_name": "startAbility"}'

# 构建知识库
doc-solution build-kb --input ./customer-inputs/ --name "华为-HarmonyOS"
```

## 核心命令

| 命令 | 功能 | 当前状态 |
|------|------|----------|
| `doc-solution check` | 质量检查（格式/风格/结构/代码） | 🚧 开发中 |
| `doc-solution generate` | 基于模板生成文档内容 | 🚧 开发中 |
| `doc-solution build-kb` | 从客户输入构建知识库 | 🚧 开发中 |
| `doc-solution resolve` | 问题单解析和修复 | 📅 规划中 |
| `doc-solution scan` | 周期性维护扫描 | 📅 规划中 |

## 项目文档

| 文档 | 说明 |
|------|------|
| `AGENTS.md` | AI Agent 维护指南 |
| `ROADMAP.md` | 开发路线图 |
| `DEVELOPMENT_LOG.md` | 开发进度记录 |
| `EVOLUTION_STRATEGY.md` | 演进策略和架构决策 |
| `schemas/` | JSON Schema 定义 |

## 技术栈

- **Python 3.11+** — 核心语言
- **Vale** — 文档 Lint 规则引擎
- **Jinja2** — 模板引擎
- **click** — CLI 框架
- **markdown-it-py** — Markdown 解析
## 项目状态

当前阶段：**Phase 1 - CLI 工具集**
