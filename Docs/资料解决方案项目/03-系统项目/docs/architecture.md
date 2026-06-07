---
audience: ai-agent
priority: high
purpose: 供 AI Agent 理解的技术架构概览
category: architecture
last-updated: 2026-06-03
---

# 系统架构

> 资料解决方案系统的技术架构

---

## 概览

资料解决方案系统是一个**文档开发全链路解决方案**，专为 AI Agent 集成而设计。系统分为三层：

```
+---------------------------------------------------+
|                   AI Agent                         |
|  (OpenCode, Cline, Claude Code 等)                 |
+---------------------------------------------------+
          |  stdin/stdout (MCP)  |  CLI (shell)
          v                     v
+-------------------+  +-------------------+
|   MCP Server      |  |   CLI 工具        |
|   (第二阶段)       |  |   (第一阶段)       |
+-------------------+  +-------------------+
          |                     |
          +----------+----------+
                     v
+-----------------------------------+
|            引擎层                  |
|  +--------+  +-------+  +------+  |
|  | 解析器  |  | 规则   |  | 检查  |  |
|  | (MD)   |  | (Vale)|  | 报告  |  |
|  +--------+  +-------+  +------+  |
+-----------------------------------+
                     |
          +----------+----------+
                     v
+-----------------------------------+
|            知识库                  |
|  (规则/ 模板/ 术语/               |
|   检查清单/ 配置)                  |
+-----------------------------------+
```

## 分层详解

### 1. CLI 层（`tools/`）

CLI 层使用 `click` 提供命令行接口。这是**第一阶段的主要交付形式**。

| 命令 | 模块 | 用途 |
|------|------|------|
| `doc-solution check` | `tools/check.py` | 质量检查 |
| `doc-solution generate` | `tools/generate.py` | 内容生成 |
| `doc-solution build-kb` | `tools/build_kb.py` | 知识库构建 |
| `doc-solution-mcp` | `mcp/server.py` | MCP 服务端（stdio） |

每个命令暴露一个 `run_*()` 程序化 API（如 `run_check()`、`run_generate()`、`run_build_kb()`），在 CLI 和 MCP Server 之间共享。

### 2. MCP 层（`mcp/`）

MCP 层实现基于 stdio 的 Model Context Protocol，用于 AI Agent 集成。

| 文件 | 用途 |
|------|------|
| `mcp/protocol.py` | JSON-RPC 2.0 协议 + stdio 传输 |
| `mcp/server.py` | 工具注册和调度 |

**协议**：JSON-RPC 2.0，带 Content-Length 头部帧。

**暴露的工具**：

| 工具名称 | 说明 | 包装函数 |
|----------|------|---------|
| `quality_check` | 运行质量检查 | `tools.check.run_check()` |
| `generate_content` | 从模板生成内容 | `tools.generate.run_generate()` |
| `build_knowledge` | 构建知识库 | `tools.build_kb.run_build_kb()` |

### 3. 引擎层（`engine/`）

引擎层包含所有核心逻辑。它没有 CLI 依赖，可以直接导入。

| 模块 | 组件 | 职责 |
|------|------|------|
| `engine/parser/md_parser.py` | MDParser | Markdown 解析，标题/代码块/链接提取，层级验证 |
| `engine/rule_engine/vale_adapter.py` | ValeAdapter | Vale CLI 封装，JSON 输出解析，优雅降级 |
| `engine/checker/reporter.py` | CheckReport | 统一报告格式，JSON/文本输出，评分 |
| `engine/knowledge/` |（预留）| 未来：智能知识库引擎 |
| `engine/template_engine/` |（预留）| 未来：模板引擎适配器 |

### 4. 知识库（`knowledge/`）

知识库存储客户特定的配置和资产。详见 `docs/knowledge-base.md`。

## 数据流

### 质量检查流程

```
用户/工具 -> run_check(target, check_type)
  |
  +-> MDParser.parse()  -- 结构检查
  |     +-> check_heading_hierarchy()
  |
  +-> ValeAdapter.check()  -- 风格/格式检查
  |     +-> vale CLI（JSON 输出）
  |     +-> 优雅降级（Vale 未找到时）
  |
  +-> 内置检查  -- 格式检查
  |     +-> 代码块语言标注
  |     +-> 段落长度
  |
  +-> CheckReport  -- 合并所有结果
        +-> to_json() / to_text()
```

### 内容生成流程

```
用户/工具 -> run_generate(template, params)
  |
  +-> Jinja2 Environment + FileSystemLoader
  +-> 模板查找（名称/目录/文件）
  +-> 使用参数渲染
  +-> 可选：auto_check -> MDParser -> CheckReport
  +-> 返回 (content, report)
```

### 知识库构建流程

```
用户/工具 -> run_build_kb(input_dir, name)
  |
  +-> 扫描输入文件（.md .yaml .json .py .ts 等）
  +-> 分析文档风格（最多 50 个文件）
  |     +-> 标题统计
  |     +-> 段落统计
  +-> 生成 Vale 配置（.vale.ini + styles）
  +-> 注册输入中的模板
  +-> 生成 config.yaml
  +-> 生成 style-profile.yaml
```

## 设计原则

1. **本地优先**：所有核心功能离线运行。零网络依赖。Vale 二进制内置在 `knowledge/vale.exe` 供内网使用。
2. **无 LLM 依赖**：系统为 AI Agent 提供工具，本身不包含 LLM 功能。
3. **优雅降级**：Vale 是可选的。没有它，结构/格式检查仍然有效。
4. **CLI/MCP 双模式**：每个 `run_*()` 函数在 CLI 和 MCP 中行为完全一致。
5. **Python 3.6 兼容**：不使用 3.7+ 语法特性。
6. **GBK 安全**：输出中不含 emoji 或非 ASCII 字符。
