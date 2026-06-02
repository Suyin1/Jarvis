# MCP Server Guide

> How to use the Doc Solution MCP Server for AI Agent integration

---

## What is MCP

The Model Context Protocol (MCP) is an open standard that enables AI Agents to discover and call tools provided by servers. The Doc Solution System implements an MCP server that exposes its core capabilities as tools that any MCP-compatible AI Agent can use.

## Architecture

```
+-------------------+    JSON-RPC 2.0     +-------------------+
|   AI Agent        |   over stdio       |  doc-solution-mcp |
|  (OpenCode, etc.) | <----------------> |  MCP Server       |
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

## Running the Server

### From command line

```bash
# With entry point (after pip install -e .)
doc-solution-mcp

# Or with python -m
python -m mcp.server
```

The server reads requests from stdin and writes responses to stdout. Debug logs go to stderr.

### From OpenCode (opencode.json)

The server is registered in `.opencode.json` or `opencode.json`:

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

Restart OpenCode to discover the three tools.

## Exposed Tools

### 1. quality_check

Run quality checks on documents/code.

**Input Schema:**

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `target` | string | Yes | - | File or directory path to check |
| `check_type` | string | No | `all` | `all`, `format`, `style`, `structure` |
| `vale_bin` | string | No | `vale` | Vale executable path |
| `config_path` | string | No | - | Vale config file path |

**Returns:** JSON check report (same format as `--output json` CLI output).

**Example Request:**

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

Generate document content from Jinja2 templates.

**Input Schema:**

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `template` | string | Yes | - | Template name or file path |
| `params` | object | No | `{}` | Template parameters |
| `template_dir` | string | No | `knowledge/templates` | Template directory |
| `output` | string | No | - | Output file path |
| `auto_check` | boolean | No | `true` | Auto-run quality check |

**Returns:** Generated content text, with optional quality check summary appended.

**Example Request:**

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

Build a knowledge base from customer source materials.

**Input Schema:**

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `input_dir` | string | Yes | - | Source material directory |
| `name` | string | Yes | - | Customer name |
| `output` | string | No | `knowledge` | Output directory |
| `force` | boolean | No | `false` | Overwrite existing KB |

**Returns:** Build summary with paths and stats.

**Example Request:**

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

## Protocol Details

### Transport

The server uses **stdio transport** with Content-Length header framing:

```
Content-Length: <bytes>\r\n
\r\n
<JSON-RPC 2.0 message>
```

### Supported Methods

| Method | Description |
|--------|-------------|
| `initialize` | Handshake: server returns capabilities (tools) |
| `notifications/initialized` | Client confirmation (no response) |
| `tools/list` | List all available tools |
| `tools/call` | Call a tool with arguments |

### Error Codes

| Code | Meaning |
|------|---------|
| -32700 | Parse error |
| -32600 | Invalid request |
| -32601 | Method not found |
| -32602 | Invalid params |
| -32603 | Internal error |

## Testing the Server

### Unit Tests

```bash
python -m pytest tests/test_mcp_server.py -v
```

### Manual Test with Python

```python
import subprocess, json

proc = subprocess.Popen(
    ["python", "-m", "mcp.server"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)

# Send initialize
send(proc, {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}})
resp = recv(proc)
print(resp["result"]["serverInfo"]["name"])  # "doc-solution"
```

### Test with curl-like approach

Since the server uses stdio, you cannot test with HTTP tools. Use the Python subprocess approach above.
