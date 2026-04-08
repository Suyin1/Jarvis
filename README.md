# Vale MCP Server

内网文档规范检查与修复的 MCP 服务，支持两种核心功能：

1. **规范检查** - 使用 Vale 检查文档的格式、规范、风格
2. **自动修复** - 根据检查结果或用户指令修改文档

## 项目结构

```
vale-mcp-server/
├── cmd/server/main.go       # 入口文件
├── internal/handlers/
│   └── tools.go             # MCP 工具实现
├── config/
│   ├── .vale.ini            # Vale 配置文件
│   └── styles/              # 规则文件目录
│       ├── Passive.yml
│       ├── Simplicity.yml
│       └── WeaselWords.yml
├── go.mod
└── README.md
```

## 环境准备

### 1. 安装 Go

从 https://go.dev/dl/ 下载并安装 Go 1.21+

### 2. 安装 Vale

从 https://github.com/errata-ai/vale/releases 下载二进制文件：

```bash
# Windows
# 下载 vale_X.Y.Z_windows_amd64.zip 并解压到 PATH

# 验证安装
vale --version
```

### 3. 配置 Vale 规则

将 `config/.vale.ini` 和 `config/styles/` 复制到项目根目录：

```bash
cp config/.vale.ini ./
cp -r config/styles/ ./
```

## 编译运行

```bash
# 下载依赖
go mod tidy

# 编译
go build -o vale-mcp-server ./cmd/server

# 运行
./vale-mcp-server
```

## AI 客户端配置

### Claude Desktop

编辑 `~/Library/Application Support/Claude/claude_desktop_config.json`：

```json
{
  "mcpServers": {
    "vale-mcp": {
      "command": "/path/to/vale-mcp-server",
      "env": {
        "VALE_ALLOWED_DIR": "/path/to/allowed/docs"
      }
    }
  }
}
```

### VS Code (Cline)

在 VS Code 设置中添加：

```json
{
  "mcpServers": {
    "vale-mcp": {
      "command": "vale-mcp-server",
      "env": {
        "VALE_ALLOWED_DIR": "."
      }
    }
  }
}
```

## 使用示例

### 检查文档

```
请帮我检查 docs/guide.md 文件的写作规范
```

### 修复被动语态

```
请帮我修复 docs/api.md 中的所有被动语态问题
```

### 自定义替换

```
请把所有"utilize"替换为"use"
```

## 安全说明

- 使用 `VALE_ALLOWED_DIR` 环境变量限制可访问目录
- 建议配合 AI 客户端的审批机制使用
- 所有文件操作都会记录日志

## 工具列表

| 工具 | 功能 |
|------|------|
| `check_docs` | 检查文档规范问题 |
| `fix_docs` | 根据指令修复文档 |
| `vale_status` | 检查 Vale 安装状态 |
