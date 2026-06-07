---
audience: ai-agent
priority: high
purpose: MCP Server 设置、工具和协议细节，用于 AI Agent 集成
category: reference
last-updated: 2026-06-03
---

# MCP Server 指南

> 如何使用资料解决方案 MCP Server 进行 AI Agent 集成

---

## 什么是 MCP

模型上下文协议（Model Context Protocol，MCP）是一个开放标准，使 AI Agent 能够发现并调用由服务端提供的工具。资料解决方案系统实现了一个 MCP 服务端，将其核心能力以工具形式暴露给任何兼容 MCP 的 AI Agent 使用。

## 架构

```
+-------------------+    JSON-RPC 2.0     +-------------------+
|   AI Agent        |   基于 stdio        |  doc-solution-mcp |
|  (OpenCode 等)    | <-----------------> |  MCP Server       |
+-------------------+                     +-------------------+
                                                  |
                                    +-------------+-------------+
                                    |             |             |
                                    v             v             v
                              quality_check generate_content build_knowledge
                                    |             |             |
                                    v             v             v
                              run_check()  run_generate()  run_build_kb()
```

## 运行服务端

### 命令行方式

```bash
# 使用入口点（pip install -e . 之后）
doc-solution-mcp

# 或使用 python -m
python -m mcp.server
```

服务端从 stdin 读取请求，向 stdout 写入响应。调试日志输出到 stderr。

### 从 OpenCode 启动（opencode.json）

服务端注册在 `.opencode.json` 或 `opencode.json` 中：

```json
{
  "mcp": {
    "doc-solution-mcp": {
      "type": "local",
      "command": ["python", "-m", "mcp.server"],
      "cwd": "path/to/03-system-project",
      "enabled": true
    }
  }
}
```

重启 OpenCode 即可发现三个工具。

## 暴露的工具

### 1. quality_check

对文档/代码运行质量检查。

**输入 Schema：**

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `target` | string | 是 | - | 要检查的文件或目录路径 |
| `check_type` | string | 否 | `all` | `all`、`format`、`style`、`structure` |
| `vale_bin` | string | 否 | `vale` | Vale 可执行文件路径 |
| `config_path` | string | 否 | - | Vale 配置文件路径 |

**返回：** JSON 检查报告（与 `--output json` CLI 输出格式相同）

**示例请求：**

```json
{
  "method": "tools/call",
  "params": {
    "name": "quality_check",
    "arguments": {
      "target": "./docs/sample.md",
      "check_type": "all"
    }
  }
}
```

### 2. generate_content

从 Jinja2 模板生成文档内容。

**输入 Schema：**

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `template` | string | 是 | - | 模板名称或文件路径 |
| `params` | object | 否 | `{}` | 模板参数 |
| `template_dir` | string | 否 | `knowledge/templates` | 模板目录 |
| `output` | string | 否 | - | 输出文件路径 |
| `auto_check` | boolean | 否 | `true` | 自动运行质量检查 |

**返回：** 生成的内容文本，附带可选的质检查摘要。

**示例请求：**

```json
{
  "method": "tools/call",
  "params": {
    "name": "generate_content",
    "arguments": {
      "template": "api-ref",
      "params": {
        "api_name": "startAbility",
        "declaration": "function startAbility(options: StartAbilityOptions): Promise<void>;"
      }
    }
  }
}
```

### 3. build_knowledge

从客户源材料构建知识库。

**输入 Schema：**

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `input_dir` | string | 是 | - | 源材料目录 |
| `name` | string | 是 | - | 客户名称 |
| `output` | string | 否 | `knowledge` | 输出目录 |
| `force` | boolean | 否 | `false` | 覆盖现有知识库 |

**返回：** 构建摘要，包含路径和统计信息。

**示例请求：**

```json
{
  "method": "tools/call",
  "params": {
    "name": "build_knowledge",
    "arguments": {
      "input_dir": "./customer-inputs/",
      "name": "Huawei-HarmonyOS",
      "force": true
    }
  }
}
```

## 协议细节

### 传输方式

服务端使用 **stdio 传输**，带 Content-Length 头部帧：

```
Content-Length: <字节数>\r\n
\r\n
<JSON-RPC 2.0 消息>
```

### 支持的方法

| 方法 | 说明 |
|------|------|
| `initialize` | 握手：服务端返回能力（工具列表） |
| `notifications/initialized` | 客户端确认（无响应） |
| `tools/list` | 列出所有可用工具 |
| `tools/call` | 使用参数调用工具 |

### 错误码

| 编码 | 含义 |
|------|------|
| -32700 | 解析错误 |
| -32600 | 无效请求 |
| -32601 | 方法未找到 |
| -32602 | 无效参数 |
| -32603 | 内部错误 |

## 测试服务端

### 单元测试

```bash
python -m pytest tests/test_mcp_server.py -v
```

### 使用 Python 手动测试

```python
import subprocess, json

proc = subprocess.Popen(
    ["python", "-m", "mcp.server"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)

# 发送 initialize
send(proc, {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}})
resp = recv(proc)
print(resp["result"]["serverInfo"]["name"])  # "doc-solution"
```

### 类 curl 方式测试

由于服务端使用 stdio，无法使用 HTTP 工具测试。请使用上述 Python subprocess 方式。
