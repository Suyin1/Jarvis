# 内网开发与部署指南

本文档说明在内网环境下开发、部署和扩展 Jarvis 项目的相关事项。

## 内网迁移

### 部署文件清单

将以下文件复制到内网即可运行（无需任何外部依赖）：

```
Docs/
├── vale/vale-3.14.1/vale-intranet.exe     # Vale 二进制 (~38MB)
├── mcp/vale-mcp-server/bin/vale-mcp-server.exe  # MCP Server 二进制
└── mcp/vale-mcp-server/config/            # 配置文件和规则
    ├── .vale.ini                          # 主配置文件
    └── styles/                            # 自定义规则
        ├── Passive.yml
        ├── Simplicity.yml
        └── WeaselWords.yml
```

### 运行方式

1. **设置 PATH** - 将 `vale-intranet.exe` 所在目录加入系统 PATH
2. **配置 MCP** - 在 AI 客户端中配置 MCP Server 路径（见下方 AI 客户端配置）

---

## 内网二次开发

### 问题：如果要在内网修改代码，需要什么环境？

**答案：需要 Go 语言环境。**

由于以下原因，内网二次开发必须具备 Go 编译环境：

| 原因 | 说明 |
|------|------|
| Vale 是 Go 编写的 | Vale 核心引擎使用 Go 开发，修改引擎需要 Go |
| MCP Server 使用 Go | 新增 MCP 工具需要重新编译 |
| vendor 已包含 | 项目已携带所有依赖，但编译仍需 Go 工具链 |

### 内网开发环境配置

如果需要在内网进行二次开发，需要准备：

1. **Go 1.21+** - 编译器
   - 可预先下载 `go1.26.2.windows-amd64.msi` 带入内网
   - 位置：`Docs/vale/go1.26.2.windows-amd64.msi`

2. **GCC (可选)** - 仅 Vale 源码编译需要
   - 推荐 MSYS2/MinGW-w64
   - 用于编译 Vale CGO 依赖

### 编译 Vale 内网版

```powershell
cd Docs/vale/vale-3.14.1
# 需要 gcc (MinGW)
go build -ldflags="-s -w" -o vale-intranet.exe ./cmd/vale
```

### 编译 MCP Server

```powershell
cd Docs/mcp/vale-mcp-server

# 本地 MCP 模式 (stdio)
go build -o bin/vale-mcp-server.exe ./cmd/server

# 远程 MCP 模式 (HTTP)
go build -tags http -o bin/vale-mcp-server-http.exe ./cmd/server
```

### 注意事项

- **无网络依赖**：Go 编译使用本地 vendor 目录，无需 internet 访问
- **交叉编译**：可在外网 Windows 环境编译后，将二进制传入内网
- **静态编译**：建议使用 `-ldflags="-s -w"` 减小二进制体积

---

## 架构说明：本地 MCP vs 远程 MCP

### 当前架构：本地 MCP（已实现）

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

**工作原理**：
- MCP Server 作为独立进程运行，通过 stdio 与 AI 客户端通信
- 用户必须下载并配置 MCP Server 到本地
- Vale 和 MCP Server 都运行在用户本地机器

**用户成本**：
- 需要下载 2 个二进制文件
- 需要配置 AI 客户端的 MCP 路径
- 需要理解基本概念（CLI、PATH、环境变量）

---

### 目标架构：远程 MCP（已实现）

```
┌─────────────────┐         ┌──────────────────┐
│   AI 客户端      │  HTTP   │   内网服务器       │
│ (Claude Desktop) │◄───────►│  ┌────────────┐  │
│                 │  SSE    │  │MCP Server  │  │
│                 │         │  └────────────┘  │
└─────────────────┘         │        │         │
                            │        ▼         │
                            │  ┌────────────┐  │
                            │  │Vale CLI    │  │
                            │  └────────────┘  │
                            └──────────────────┘
```

**优势**：
- 用户无需安装任何二进制文件
- 规则和配置集中管理，实时更新
- 用户通过 URL 访问服务（AI 客户端支持 HTTP MCP）
- 适合团队统一协作

**使用方法**：

1. **启动远程 MCP Server**：
```powershell
# 使用已编译的 HTTP 版本
cd Docs/mcp/vale-mcp-server/bin
.\vale-mcp-server-http.exe

# 或指定参数
$env:VALE_ALLOWED_DIR="E:\ai\Jarvis\Docs"
$env:MCP_SERVER_PORT="8080"
$env:MCP_API_KEY="your-secure-key-min-32-chars"
.\vale-mcp-server-http.exe
```

2. **AI 客户端配置**（以 Claude Desktop 为例）：
```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "http://localhost:8080/mcp",
      "env": {
        "MCP_API_KEY": "your-secure-key-min-32-chars"
      }
    }
  }
}
```

3. **安全配置（必填）**：

| 环境变量 | 说明 | 示例 |
|---------|------|------|
| `VALE_ALLOWED_DIR` | 允许访问的目录 | `E:\ai\Jarvis\Docs` |
| `MCP_API_KEY` | API Key（必填，≥32字符） | `abc123...` |
| `MCP_SERVER_PORT` | 端口（默认 8080） | `8080` |

**安全特性**：
- ✅ API Key 认证（Header: `X-API-Key` 或 Query: `api_key`）
- ✅ 目录遍历防护（路径校验 + 符号链接检查）
- ✅ 命令注入防护（参数化执行，无 shell）
- ✅ 速率限制（60 req/min/IP）
- ✅ 日志脱敏（敏感字段截断）
- ✅ 本地绑定（默认 127.0.0.1，仅监听本机）

> ⚠️ **安全提示**：远程 MCP 涉及网络传输，必须确保数据安全。详见 [远程 MCP 安全分析报告](REMOTE_MCP_SECURITY.md)。

---

## 常见问题

### Q1: 内网没有 Go 环境，能否修改规则？

**可以**。Vale 规则文件（`.yml`）是纯文本 YAML，修改规则无需编译：

1. 编辑 `config/styles/*.yml` 文件
2. 修改 `.vale.ini` 中的规则引用
3. 无需重新编译，立即生效

### Q2: 内网没有 Go 环境，能否添加新功能？

**不可以**。新增 MCP 工具或修改 Vale 核心逻辑需要重新编译：

- 添加新工具 → 修改 `internal/handlers/tools.go` → 重新编译 MCP Server
- 修改检查逻辑 → 修改 Vale 源码 → 重新编译 Vale

### Q3: 如何在只读文件系统部署？

1. 将二进制文件和配置复制到目标位置
2. 配置 `VALE_ALLOWED_DIR` 环境变量指向文档目录
3. 只读挂载配置文件，确保安全

---

## 版本信息

- Vale: 3.14.1 (内网安全版)
- MCP Server: 1.0.0
- Go: 1.21+ (开发环境)
