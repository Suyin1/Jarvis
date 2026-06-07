---
audience: both
priority: high
purpose: 客户和 AI Agent 的入口文档与文档索引
category: guide
last-updated: 2026-06-03
---

# 资料解决方案系统

> 文档开发全链路解决方案：质量检查、内容生成、知识库构建

---

## 快速开始

```bash
# 检查一切是否正常
python -m pytest tests/ -v

# 检查文档
python -m tools.cli check --target examples/sample-docs/sample-guide.md

# 从模板生成内容
python -m tools.cli generate --template api-ref --params '{"api_name": "startAbility"}'

# 构建知识库
python -m tools.cli build-kb --input examples/sample-docs/ --name "示例"
```

## 按读者分类的文档

### 面向客户

| 文档 | 说明 |
|------|------|
| `PRODUCT_GUIDE.md` | 产品概览：能力、工作流程、快速入门 |
| `docs/customer/SECURITY.md` | 安全与隐私：零网络证明、常见问题 |

### 面向 AI Agent / 技术用户

| 文档 | 类别 | 说明 |
|------|------|------|
| `USAGE.md` | 参考 | CLI + MCP 工具参考，所有选项和示例 |
| `AGENTS.md` | 指南 | AI Agent 维护工作流和规范 |
| `docs/architecture.md` | 架构 | 系统分层、数据流、设计原则 |
| `docs/cli-tools.md` | 参考 | 完整 CLI 命令参考 |
| `docs/mcp-server.md` | 参考 | MCP Server 设置、工具、协议细节 |
| `docs/knowledge-base.md` | 指南 | 知识库构建、配置、多客户维护 |
| `docs/kb-construction-guide.md` | 指南 | 知识库内容构建：Vale 规则、术语、测试转换 |
| `docs/vale-checking.md` | 参考 | Vale 集成、规则系统、离线证明 |

### 面向所有人

| 文档 | 说明 |
|------|------|
| `ROADMAP.md` | 开发路线图和里程碑 |
| `DEVELOPMENT_LOG.md` | 变更历史 |
| `02-设计方案/演进策略与架构决策记录.md` | 演进策略和架构决策 |

## 核心能力

### 1. 质量检查

```bash
doc-solution check --target <路径> [--check-type all|structure|format|style]
```

三个检查级别：
- **结构**：标题层级、必需章节（内置，无依赖）
- **格式**：段落长度、代码块标注（内置 + 可选 Vale）
- **风格**：术语、规范（通过 Vale，可选）

详见 `docs/vale-checking.md`。

### 2. 内容生成

```bash
doc-solution generate --template <名称> --params '<json>'
```

使用 Jinja2 模板，生成后自动进行质量检查。
模板按名称、目录或文件路径解析。
详见 `docs/cli-tools.md` 的模板参考。

### 3. 知识库构建

```bash
doc-solution build-kb --input <目录> --name <客户名称>
```

分析客户源材料，构建知识库，包含：
- 文档风格档案（标题、段落统计）
- Vale 配置和风格规则
- 注册的模板
- 术语表和检查清单模板
详见 `docs/knowledge-base.md`。

### 4. AI Agent 集成（MCP）

系统将所有能力以 MCP 工具形式暴露给 AI Agent：

```json
// 在 opencode.json 中配置
{
  "mcp": {
    "doc-solution-mcp": {
      "type": "local",
      "command": ["python", "-m", "mcp.server"],
      "enabled": true
    }
  }
}
```

可用工具：`quality_check`、`generate_content`、`build_knowledge`。
详见 `docs/mcp-server.md`。

## 项目结构

```
.
|-- README.md                    # 本文件
|-- docs/                        # 技术文档
|   |-- architecture.md
|   |-- knowledge-base.md
|   |-- vale-checking.md
|   |-- cli-tools.md
|   |-- mcp-server.md
|-- AGENTS.md                    # AI Agent 维护指南
|-- USAGE.md                     # AI Agent 使用指南
|-- PRODUCT_GUIDE.md             # 客户使用指南
|-- TESTING_GUIDE.md             # 回归测试指南
|-- ROADMAP.md                   # 开发路线图
|-- DEVELOPMENT_LOG.md           # 变更历史
|
|-- engine/                      # 核心引擎（Python）
|   |-- parser/                  #   Markdown 解析器
|   |-- rule_engine/             #   Vale 适配器
|   |-- checker/                 #   报告生成器
|   +-- knowledge/               #   知识库引擎（预留）
|-- tools/                       # CLI 入口
|   |-- cli.py                   #   主入口
|   |-- check.py                 #   质量检查
|   |-- generate.py              #   内容生成
|   +-- build_kb.py              #   知识库构建
|-- mcp/                         # MCP Server
|   |-- protocol.py              #   JSON-RPC 2.0 协议
|   +-- server.py                #   服务端 + 工具注册
|-- knowledge/                   # 默认知识库
|   |-- config.yaml
|   |-- rules/                   # Vale + 自定义规则
|   |-- templates/               # Jinja2 模板
|   |-- glossary/                # 术语
|   +-- checklist/               # 质量检查清单
|-- schemas/                     # JSON Schema 定义
|-- tests/                       # 38 个通过测试
+-- examples/                    # 示例配置
```

## 技术细节

- **Python 3.6+** 兼容（不使用 3.7+ 语法）
- **零网络**依赖（全部离线）
- **无 LLM** 依赖（工具包，非 AI）
- **Vale 可选**（优雅降级，内置二进制文件 `knowledge/vale.exe`）
- **Windows GBK** 编码安全

## 当前状态

- Phase 1（CLI）：完成 — 3 个可用命令
- Phase 2（MCP）：完成 — 3 个工具通过 JSON-RPC 2.0
- 38 个测试通过
- 版本 0.2.0
