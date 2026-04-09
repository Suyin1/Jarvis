# Vale MCP Server 使用指南

本文档详细介绍 Vale MCP Server 的两种部署模式：本地模式和远程模式，以及如何开发和扩展。

## 目录

1. [快速开始](#快速开始)
2. [架构概述](#架构概述)
3. [本地 MCP 模式（stdio）](#本地-mcp-模式stdio)
4. [远程 MCP 模式（HTTP）](#远程-mcp-模式http)
5. [AI 客户端配置](#ai-客户端配置)
6. [开发指南](#开发指南)
7. [安全配置](#安全配置)
8. [故障排除](#故障排除)

---

## 快速开始

### 方式一：本地模式（用户端）

用户无需启动服务器，直接在 AI 客户端配置 MCP 路径即可。

---

## 快速开始

### 本地模式最小配置

只需 2 个文件即可使用 MCP 服务：

```
E:\tools\
├── vale.exe              # Vale 检查引擎（38MB）
└── vale-mcp-server.exe   # MCP 服务器（9MB）
```

#### 步骤 1：准备文件

1. **下载 Vale**：将 `vale-intranet.exe` 重命名为 `vale.exe`，放入 `E:\tools\`
2. **下载 MCP Server**：将 `vale-mcp-server.exe` 放入 `E:\tools\`

#### 步骤 2：配置 PATH（让系统能找到 Vale）

```powershell
# 方式1：复制到系统目录（推荐）
copy "E:\tools\vale.exe" "C:\Windows\System32\vale.exe"

# 方式2：添加到 PATH
setx PATH "%PATH%;E:\tools"
```

验证：
```powershell
vale --version
```

#### 步骤 3：配置 AI 客户端

在 AI 客户端的配置文件中添加：

```json
{
  "mcpServers": {
    "vale-local": {
      "command": "E:\\tools\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\你的项目目录"
      }
    }
  }
}
```

#### 步骤 4：放置 .vale.ini（配置检查规则）

Vale 有两种规则来源：

##### 方式1：使用内置规则（推荐，最简单）

**注意**：Vale 内网版禁用了 `vale sync` 命令，**无法自动下载远程样式包**（如 Microsoft, Google）。

内置规则只有 **Vale 自己的 4 个基础规则**：

| 规则 | 说明 |
|------|------|
| `Vale.Spelling` | 拼写检查（需要配置 Vocab） |
| `Vale.Terms` | 术语一致性（需要配置 Vocab） |
| `Vale.Repetition` | 重复单词检测 |
| `Vale.Avoid` | 避免词汇（需要配置 Vocab） |

可以直接使用 `.vale.ini` 配置：

```ini
# .vale.ini
[*.md]
Vale.Spelling = YES          # 启用拼写检查
Vale.Terms = YES             # 启用术语检查（需要 Vocab）
Vale.Repetition = YES        # 检测重复单词
```

##### 方式2：使用自定义规则（需要手动下载样式包）

由于内网版无法使用 `vale sync` 自动下载样式包（如 Microsoft, Google），需要手动获取：

1. **手动下载样式包**：从 [Vale styles](https://github.com/errata-ai?type=source) GitHub 下载
2. **放到本地目录**：

```
项目目录/
├── .vale.ini
└── styles/
    └── Microsoft/
        └── ...（手动下载的规则文件）
```

```ini
# .vale.ini
[General]
StylesPath = styles         # 指向本地规则目录

[*.md]
BasedOnStyles = Microsoft  # 使用自定义规则
```

**自定义规则文件格式**（`.yml`）：

```yaml
# styles/myrule/SimpleWords.yml
extends: substitution
message: "建议使用 '%s' 代替 '%s'"
level: warning
swap:
  accelerate: speed up
  collaborate: work together
```

---

详细的自定义规则开发说明见 [VALE_CONFIG_GUIDE.md](../../vale/VALE_CONFIG_GUIDE.md)。

#### 使用方法

在 AI 对话中直接请求检查：

```
请检查我的文档 E:\项目\README.md 的风格问题
```

---

### 方式二：远程模式（服务器部署）

```powershell
# 1. 服务器启动
cd Docs/mcp/vale-mcp-server/bin
$env:VALE_ALLOWED_DIR="E:\ai\Jarvis\Docs"
$env:MCP_API_KEY="your-secure-key-min-32-chars"
.\vale-mcp-server-http.exe

# 2. AI 客户端连接（见下方 AI 客户端配置章节）
```

---

## 架构概述

### 本地 MCP 模式

```
┌─────────────────┐         ┌──────────────────┐
│   AI 客户端      │         │   用户电脑        │
│ (Claude Desktop) │◄───────►│  ┌────────────┐  │
│                 │  stdio   │  │MCP Server  │  │
│                 │         │  └────────────┘  │
└─────────────────┘         └──────────────────┘
                                   │
                                   ▼
                            ┌────────────┐
                            │Vale CLI    │
                            └────────────┘
```

- MCP Server 作为子进程运行，通过 stdio 与 AI 客户端通信
- 用户需要下载并配置两个二进制文件
- 无网络传输，安全性高

### 远程 MCP 模式

```
┌─────────────────┐         ┌──────────────────┐
│   AI 客户端      │  HTTP   │   内网服务器       │
│ (Claude Desktop) │◄───────►│  ┌────────────┐  │
│                 │  SSE    │  │MCP Server  │  │
│                 │         │  └────────────┘  │
└─────────────────┘         └──────────────────┘
                                    │
                                    ▼
                             ┌────────────┐
                             │Vale CLI    │
                             └────────────┘
```

- MCP Server 作为 HTTP 服务运行
- 用户无需安装二进制，通过 URL 访问
- 需要配置 API Key 认证

---

## 本地 MCP 模式（stdio）

### 使用条件

- 用户本地安装 Vale CLI (`vale-intranet.exe`)
- 用户下载 MCP Server 二进制 (`vale-mcp-server.exe`)
- AI 客户端支持本地 MCP 配置

### 使用步骤

#### 步骤 1：准备二进制

```
Docs/
├── vale/vale-3.14.1/vale-intranet.exe  # 放入系统 PATH
└── mcp/vale-mcp-server/bin/vale-mcp-server.exe
```

#### 步骤 2：配置 Vale

Vale 需要在项目根目录找到 `.vale.ini` 配置文件才能运行。

**配置方式**：将 `.vale.ini` 放在你希望检查的文档所在目录。Vale 会自动搜索当前目录及父目录。

**示例 `.vale.ini`**：

```ini
# .vale.ini
[*.md]
Vale.Spelling = NO
Microsoft.FirstPerson = NO
Microsoft.We = YES
```

**配置说明**：

| 配置项 | 说明 |
|--------|------|
| `[*.md]` | 匹配 Markdown 文件 |
| `Vale.Spelling = NO` | 禁用拼写检查 |
| `Microsoft.FirstPerson = NO` | 禁用第一人称检查 |
| `Microsoft.We = YES` | 启用 we 词汇检查 |

详细的 Vale 配置说明见 [VALE_CONFIG_GUIDE.md](../../vale/VALE_CONFIG_GUIDE.md)。

#### 步骤 3：配置 AI 客户端

根据不同的 AI 客户端，配置方式如下：

##### Claude Desktop (Windows)

编辑 `AppData\Roaming\Claude\claude_desktop_config.json`：

```json
{
  "mcpServers": {
    "vale-local": {
      "command": "E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis\\Docs"
      }
    }
  }
}
```

##### Cursor

编辑 `AppData\Roaming\Cursor\User\globalStorage\anthropic-aichat\mcp_settings.json`：

```json
{
  "mcpServers": {
    "vale-local": {
      "command": "E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis\\Docs"
      }
    }
  }
}
```

##### Windsurf (Codeium)

编辑 `AppData\Roaming\Windsurf\user\globalStorage\codeium-extensions\mcp_settings.json`：

```json
{
  "mcpServers": {
    "vale-local": {
      "command": "E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis\\Docs"
      }
    }
  }
}
```

##### Cline (VS Code 插件)

在 Cline 设置中添加：

```json
{
  "mcpServers": {
    "vale-local": {
      "command": "E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis\\Docs"
      }
    }
  }
}
```

##### Continue (VS Code 插件)

在 `.vscode/settings.json` 中添加：

```json
{
  "continue.mcpServers": {
    "vale-local": {
      "command": "E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe",
      "env": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis\\Docs"
      }
    }
  }
}
```

##### OpenCode

在项目根目录创建 `opencode.json`：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "vale-mcp-server": {
      "type": "local",
      "command": ["E:\\ai\\Jarvis\\Docs\\mcp\\vale-mcp-server\\bin\\vale-mcp-server.exe"],
      "environment": {
        "VALE_ALLOWED_DIR": "E:\\ai\\Jarvis"
      },
      "enabled": true
    }
  }
}
```

验证配置：

```powershell
opencode mcp list
```

使用工具（在对话中直接请求）：

```
请检查 Docs/README.md 的文档风格问题
```

#### 步骤 3：重启 AI 客户端

重启后，MCP Server 会自动启动并注册工具。

#### 步骤 4：使用工具

在 AI 对话中，可以使用以下工具：

```
请检查 Docs/README.md 的文档风格
```

---

## 远程 MCP 模式（HTTP）

### 使用条件

- 服务器部署 MCP Server（HTTP 版本）
- 用户配置 AI 客户端连接远程 URL
- 需要 API Key 认证（可选但推荐）

### 服务器部署

#### 方式一：直接运行

```powershell
# 启动服务器
cd Docs/mcp/vale-mcp-server/bin

# 必需：设置允许访问的目录
$env:VALE_ALLOWED_DIR="E:\ai\Jarvis\Docs"

# 必需：设置 Vale 配置文件（.vale.ini 放在文档目录中）
# Vale 会在 E:\ai\Jarvis\Docs 目录找到 .vale.ini

# 必需：设置 API Key（至少 32 字符）
$env:MCP_API_KEY="your-very-secure-key-min-32-characters"

# 可选：设置端口（默认 8080）
$env:MCP_SERVER_PORT="8080"

# 启动
.\vale-mcp-server-http.exe
```

#### Vale 配置文件位置

在 HTTP/远程模式下：

```
服务器端：
┌─────────────────────────────┐
│  服务器 (MCP 服务)           │
│  ├── vale-mcp-server-http.exe
│  └── Docs/                   │  ← 客户端调用 check_docs 时
│      ├── .vale.ini           │     服务器在此目录找配置
│      └── README.md           │     并执行 Vale 检查
└─────────────────────────────┘
```

**关键点**：
- `.vale.ini` 只需要在**服务器**的文档目录放一份
- 客户端不需要配置 `.vale.ini`
- 客户端传入文件路径，服务器在本地目录查找并检查
- `VALE_ALLOWED_DIR` 限制了服务器允许访问的目录（安全限制）

#### 方式二：使用脚本

创建启动脚本 `start-mcp-server.ps1`：

```powershell
# start-mcp-server.ps1

$ErrorActionPreference = "Stop"

# 配置（根据实际情况修改）
$AllowedDir = "E:\ai\Jarvis\Docs"
$ApiKey = "your-very-secure-key-min-32-characters"
$Port = "8080"

# 设置环境变量
$env:VALE_ALLOWED_DIR = $AllowedDir
$env:MCP_API_KEY = $ApiKey
$env:MCP_SERVER_PORT = $Port

# 启动服务
$ServerPath = Join-Path $PSScriptRoot "bin\vale-mcp-server-http.exe"
& $ServerPath
```

#### 方式三：Docker 部署

创建 `Dockerfile`：

```dockerfile
FROM golang:1.25-alpine AS builder

WORKDIR /app
COPY Docs/mcp/vale-mcp-server/ .
RUN go build -tags http -o vale-mcp-server-http ./cmd/server

FROM alpine:latest
RUN apk add --no-cache ca-certificates
COPY --from=builder /app/vale-mcp-server-http /usr/local/bin/
COPY --from=builder /app/config/ /app/config/

ENV VALE_ALLOWED_DIR=/app/docs
ENV MCP_API_KEY=${MCP_API_KEY:-}
ENV MCP_SERVER_PORT=8080

EXPOSE 8080
CMD ["vale-mcp-server-http"]
```

构建和运行：

```bash
# 构建
docker build -t vale-mcp-server .

# 运行
docker run -d \
  -e VALE_ALLOWED_DIR=/app/docs \
  -e MCP_API_KEY=your-secure-key \
  -e MCP_SERVER_PORT=8080 \
  -p 8080:8080 \
  -v /path/to/docs:/app/docs \
  vale-mcp-server
```

### 客户端配置

#### Claude Desktop

编辑 `AppData\Roaming\Claude\claude_desktop_config.json`：

```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "http://localhost:8080/mcp",
      "env": {
        "MCP_API_KEY": "your-very-secure-key-min-32-characters"
      }
    }
  }
}
```

#### Cursor

```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "http://localhost:8080/mcp",
      "env": {
        "MCP_API_KEY": "your-very-secure-key-min-32-characters"
      }
    }
  }
}
```

#### Windsurf

```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "http://localhost:8080/mcp",
      "env": {
        "MCP_API_KEY": "your-very-secure-key-min-32-characters"
      }
    }
  }
}
```

#### Cline

```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "http://localhost:8080/mcp",
      "env": {
        "MCP_API_KEY": "your-very-secure-key-min-32-characters"
      }
    }
  }
}
```

#### 通用配置（支持 SSE）

部分客户端支持通过 SSE 端点连接：

```
http://localhost:8080/sse
```

#### OpenCode 远程模式

在项目根目录 `opencode.json` 中配置：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "vale-mcp-server": {
      "type": "remote",
      "url": "http://192.168.1.100:8080/mcp",
      "enabled": true,
      "headers": {
        "Authorization": "Bearer your-very-secure-key-min-32-characters"
      }
    }
  }
}
```

### API Key 认证机制

#### 服务器端配置

在服务器上设置环境变量：

```powershell
# 设置 API Key（必需，至少 32 字符）
$env:MCP_API_KEY="your-very-secure-key-min-32-characters"
```

#### 客户端配置方式

不同的客户端有不同的传参方式：

| 客户端 | 传参方式 | 配置示例 |
|--------|----------|----------|
| Claude Desktop | 通过 `env.MCP_API_KEY` | 见上方配置 |
| Cursor | 通过 `env.MCP_API_KEY` | 见上方配置 |
| OpenCode | 通过 `headers.Authorization` | 见上方配置 |

#### 认证方式说明

- **服务器端**：启动时读取 `MCP_API_KEY` 环境变量
- **客户端请求**：在请求头中添加 `Authorization: Bearer <key>` 或 `X-API-Key: <key>`
- **验证**：服务器比对客户端提供的 key 与配置的 key 是否一致
- **唯一性**：是，所有客户端必须使用相同的 key（服务器配置的那个）

#### 不启用 API Key

如果不需要认证，可以不设置 `MCP_API_KEY`：
```powershell
# 不设置 API Key，服务器不验证
$env:MCP_SERVER_PORT="8080"
.\vale-mcp-server-http.exe
```

**注意**：生产环境建议启用 API Key 认证。

验证配置：

```powershell
opencode mcp list
opencode mcp debug vale-mcp-server
```

---

## AI 客户端配置

### 配置对比

| 客户端 | 本地模式 | 远程模式 | 配置文件位置 |
|--------|---------|---------|------------|
| Claude Desktop | ✅ 支持 | ✅ 支持 | `%APPDATA%\Claude\claude_desktop_config.json` |
| Cursor | ✅ 支持 | ✅ 支持 | `%APPDATA%\Cursor\User\globalStorage\anthropic-aichat\mcp_settings.json` |
| Windsurf | ✅ 支持 | ✅ 支持 | `%APPDATA%\Windsurf\user\globalStorage\codeium-extensions\mcp_settings.json` |
| Cline | ✅ 支持 | ✅ 支持 | Cline 设置界面 |
| Continue | ✅ 支持 | ✅ 支持 | `.vscode/settings.json` |
| OpenCode | ✅ 支持 | ✅ 支持 | 项目根目录 `opencode.json` |

### 通用 MCP 配置原理

MCP（Model Context Protocol）是一个标准协议，所有支持 MCP 的 AI 客户端配置方式本质相同：

#### 1. 本地模式（stdio）

```json
{
  "mcpServers": {
    "服务名称": {
      "command": "可执行文件路径",
      "env": {
        "环境变量": "值"
      }
    }
  }
}
```

#### 2. 远程模式（HTTP）

```json
{
  "mcpServers": {
    "服务名称": {
      "url": "http://服务器地址:端口/mcp",
      "env": {
        "MCP_API_KEY": "认证密钥"
      }
    }
  }
}
```

**通用字段**：
- `command` / `url`：二选一，决定本地或远程模式
- `env`：传递环境变量（如目录限制、认证信息）

#### 各客户端差异

| 差异点 | 说明 |
|--------|------|
| 配置文件位置 | 有的在 AppData，有的在项目目录 |
| 配置格式 | 大多数用 JSON，部分用 JSONC |
| 远程认证 | 有的用 `env.MCP_API_KEY`，有的用 `headers.Authorization` |

### 配置字段说明

```json
{
  "mcpServers": {
    "vale-local": {              // 服务名称（可自定义）
      "command": "path/to/exe",  // 本地模式：二进制路径
      "url": "http://...",      // 远程模式：服务端点
      "env": {                  // 环境变量
        "VALE_ALLOWED_DIR": "允许访问的目录",
        "MCP_API_KEY": "认证密钥（远程模式）"
      }
    }
  }
}
```

---

## 开发指南

### 项目结构

```
Docs/mcp/vale-mcp-server/
├── cmd/
│   └── server/
│       ├── main.go           # 本地模式入口（stdio）
│       ├── main_http.go     # 远程模式入口（HTTP）
│       └── middleware.go    # 安全中间件
│
├── internal/
│   └── handlers/
│       └── tools.go         # MCP 工具实现
│
├── config/
│   ├── .vale.ini           # Vale 配置
│   └── styles/             # 自定义规则
│       ├── Passive.yml
│       ├── Simplicity.yml
│       └── WeaselWords.yml
│
├── vendor/                  # Go 依赖（离线可用）
├── go.mod                  # 模块声明
└── bin/                    # 编译输出
    ├── vale-mcp-server.exe        # 本地模式
    └── vale-mcp-server-http.exe   # 远程模式
```

### 开发环境

#### 前提条件

- Go 1.25+
- Git

#### 本地开发

```powershell
# 克隆项目后
cd Docs/mcp/vale-mcp-server

# 本地模式编译
go build -o bin/vale-mcp-server.exe ./cmd/server

# 远程模式编译
go build -tags http -o bin/vale-mcp-server-http.exe ./cmd/server

# 运行测试
go test ./...
```

### 添加新工具

#### 步骤 1：定义工具

在 `internal/handlers/tools.go` 中添加：

```go
func registerNewTool(server *mcp.Server, allowedDir string) {
    tool := mcp.Tool{
        Name:        "tool_name",
        Description: "工具描述",
        InputSchema: map[string]any{
            "type": "object",
            "properties": map[string]any{
                "param1": map[string]any{
                    "type":        "string",
                    "description": "参数说明",
                },
            },
            "required": []string{"param1"},
        },
    }

    handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
        // 实现逻辑
        return &mcp.CallToolResult{
            Content: []mcp.Content{&mcp.TextContent{Text: "结果"}},
        }, nil, nil
    }

    mcp.AddTool(server, &tool, handler)
}
```

#### 步骤 2：注册工具

在 `RegisterAllTools` 函数中添加：

```go
func RegisterAllTools(server *mcp.Server, allowedDir string) {
    absAllowedDir, _ := filepath.Abs(allowedDir)
    registerValeCheckTool(server, absAllowedDir)
    registerValeFixTool(server, absAllowedDir)
    registerValeStatusTool(server)
    registerNewTool(server, absAllowedDir)  // 添加这行
}
```

#### 步骤 3：编译部署

```powershell
# 本地模式
go build -o bin/vale-mcp-server.exe ./cmd/server

# 远程模式
go build -tags http -o bin/vale-mcp-server-http.exe ./cmd/server
```

### 修改 Vale 规则

Vale 规则文件位于 `config/styles/`，修改后无需重新编译，立即生效：

```yaml
# config/styles/Passive.yml
extends: substitution
message: "Use 'was/were' + verb form"
level: warning
ignorecase: false
swap:
  is believed to: is believed
  is said to: is said
```

---

## 安全配置

### 环境变量

| 变量名 | 说明 | 必填 | 示例 |
|--------|------|------|------|
| `VALE_ALLOWED_DIR` | 允许访问的目录 | 是 | `E:\ai\Jarvis\Docs` |
| `MCP_API_KEY` | API Key（≥32字符） | 远程模式 | `abc123...` |
| `MCP_SERVER_PORT` | 监听端口 | 否 | `8080` |

### 安全特性

| 特性 | 说明 | 默认值 |
|------|------|--------|
| API Key 认证 | 支持 Header 和 Query 参数 | 关闭 |
| 目录遍历防护 | 路径校验 + 符号链接检查 | 开启 |
| 命令注入防护 | 参数化执行，无 shell | 开启 |
| 速率限制 | 60 req/min/IP | 开启 |
| 日志脱敏 | 敏感字段截断 | 开启 |
| 本地绑定 | 仅监听 127.0.0.1 | 开启 |

### 生成安全 API Key

```powershell
# PowerShell 生成随机 Key
$bytes = New-Object byte[] 32
$rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
$rng.GetBytes($bytes)
[Convert]::ToBase64String($bytes)
```

---

## 故障排除

### 本地模式问题

#### Q: MCP Server 未启动

**检查**：
1. 二进制路径是否正确
2. `VALE_ALLOWED_DIR` 是否设置
3. Vale 是否在 PATH 中

### 为什么需要配置 PATH？

#### 作用原理

```
┌──────────────┐         ┌─────────────────┐         ┌──────────────┐
│  AI 客户端    │────────▶│  MCP Server    │────────▶│  Vale CLI   │
│              │  MCP 协议 │                 │  执行命令   │              │
└──────────────┘         └─────────────────┘         └──────────────┘
```

MCP Server 只是一个"桥梁"，它：
1. 接收 AI 客户端的请求
2. 调用系统命令 `vale` 来执行实际的文档检查
3. 返回结果给 AI

#### PATH 的作用

- `PATH` 是系统环境变量，告诉操作系统去哪里找可执行文件
- MCP Server 调用 `vale` 命令时，系统需要通过 PATH 找到 `vale.exe`
- 如果 Vale 不在 PATH 中，系统找不到它，检查就会失败

#### 不配 PATH 会怎样

| 情况 | 结果 |
|------|------|
| PATH 未配置 | MCP Server 调用 `vale` 失败，返回"找不到命令"错误 |
| 检查无法执行 | AI 无法获取文档风格检查结果 |

#### 配置 PATH 的好处

| 好处 | 说明 |
|------|------|
| 全局可用 | 配置一次，所有工具都能调用 Vale |
| 灵活部署 | Vale 可以放在任意目录，只需添加到 PATH |
| 多项目共享 | 同一个 Vale 可以服务多个项目 |

#### 解决方案

```powershell
# 方式1：复制到系统目录（推荐）
copy "E:\ai\Jarvis\Docs\vale\vale-3.14.1\vale-intranet.exe" "C:\Windows\System32\vale.exe"

# 方式2：添加到用户 PATH（仅当前用户有效）
setx PATH "%PATH%;E:\ai\Jarvis\Docs\vale\vale-3.14.1"

# 方式3：临时设置（仅当前 CMD 会话有效）
$env:PATH += ";E:\ai\Jarvis\Docs\vale\vale-3.14.1"

# 验证配置成功
vale --version
```

**解决方案**：
```powershell
# 将 vale-intranet.exe 复制到系统目录（CMD 管理员）
copy "E:\ai\Jarvis\Docs\vale\vale-3.14.1\vale-intranet.exe" "C:\Windows\System32\vale.exe"

# 或添加到用户 PATH
$env:PATH += ";E:\ai\Jarvis\Docs\vale\vale-3.14.1"

# 验证
vale --version
```

**日志**：
```
# Windows
%TEMP%\vale-mcp-server.log
```

#### Q: 工具调用失败

**检查**：
1. Vale CLI 是否可用：`vale --version`
2. 目录权限是否正确

### 远程模式问题

#### Q: 连接失败

**检查**：
1. 服务器是否启动：`curl http://127.0.0.1:8080/mcp`
2. API Key 是否正确
3. 防火墙是否阻止

#### Q: 认证失败

**检查**：
1. Header 是否正确传递：`X-API-Key`
2. Key 是否与环境变量匹配

### 调试模式

```powershell
# 启用详细日志
$env:LOG_LEVEL = "debug"
.\vale-mcp-server-http.exe
```

---

## 相关文档

- [内网部署指南](../INTRANET_DEPLOYMENT.md)
- [远程 MCP 安全分析](../REMOTE_MCP_SECURITY.md)
- [Vale 配置指南](../vale/VALE_CONFIG_GUIDE.md)

---

**文档版本**：1.0  
**更新时间**：2026-04-09  
**适用版本**：Vale MCP Server 1.0.0+