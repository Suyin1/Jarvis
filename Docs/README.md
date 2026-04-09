# Jarvis 项目文档规范检查系统

## 项目概述

Jarvis 文档规范检查系统是一套专为**内网环境**设计的文档质量检查解决方案，基于 Vale 和 MCP (Model Context Protocol) 构建。

### 核心功能

| 组件 | 功能 | 状态 |
|------|------|------|
| Vale 内网版 | 文档风格检查引擎 | ✅ 完成 |
| MCP Server | AI 助手集成层 | ✅ 完成 |
| Vale 配置生成器 | 规范转配置 | ✅ 完成 |
| 自定义规则 | 中文文档优化规则 | ✅ 完成 |

---

## 目录结构

```
Docs/
├── vale/
│   ├── vale-3.14.1/              # Vale 源码 (内网安全版)
│   │   ├── cmd/vale/              # CLI 入口
│   │   ├── internal/              # 核心库
│   │   │   ├── core/             # 配置、文件处理
│   │   │   ├── nlp/              # NLP 处理
│   │   │   ├── check/            # 检查规则
│   │   │   └── lint/             # 语法检查
│   │   ├── vendor/               # Go 依赖 (241个模块)
│   │   ├── vale-intranet.exe    # 编译好的二进制 (38MB)
│   │   ├── INTRANET_README.md   # 内网版说明
│   │   └── go.mod               # 依赖声明
│   │
│   ├── VALE_CONFIG_GUIDE.md     # Vale 配置生成指南
│   └── vale.md                  # Vale 使用说明
│
├── mcp/
│   └── vale-mcp-server/         # MCP Server 源码
│       ├── cmd/server/main.go   # 程序入口
│       ├── internal/handlers/   # MCP 工具实现
│       │   └── tools.go         # check_docs, fix_docs, vale_status
│       ├── config/              # Vale 配置
│       │   ├── .vale.ini        # 主配置文件
│       │   └── styles/          # 自定义规则
│       │       ├── Passive.yml  # 被动语态规则
│       │       ├── Simplicity.yml # 简化用词规则
│       │       └── WeaselWords.yml # 模糊词汇规则
│       ├── README.md            # MCP Server 使用说明
│       └── go.mod               # 依赖声明
│
├── skills/
│   ├── generating-vale-config.md # Vale 配置生成 Skill
│   └── (其他 Skill...)
│
├── README.md                    # 本文档
├── DEVELOPMENT_LOG.md           # 开发记录
├── SECURITY.md                 # 安全分析报告
└── 资料自动化方案v1.md           # 需求方案
```

---

## 快速开始

### 方式一：直接使用二进制

```powershell
#Vale 内网版已编译好，直接使用
Docs/vale/vale-3.14.1/vale-intranet.exe --version

#MCP Server 需要 Go 环境编译
#见下方编译说明
```

### 方式二：从源码编译

```powershell
#Vale 内网版 (需要 gcc)
cd Docs/vale/vale-3.14.1
set PATH=C:\msys64\mingw64\bin;%PATH%
go build -ldflags="-s -w" -o vale-intranet.exe ./cmd/vale

#MCP Server
cd Docs/mcp/vale-mcp-server
go build -o vale-mcp-server.exe ./cmd/server
```

---

## 组件说明

### 1. Vale 内网版

**版本**: v3.14.1  
**源码**: `Docs/vale/vale-3.14.1/`  
**二进制**: `vale-intranet.exe` (38MB)

#### 安全特性

| 功能 | 状态 |
|------|------|
| `vale sync` | ✅ 已禁用 |
| `vale install` | ✅ 已禁用 |
| `vale host-install` | ✅ 已禁用 |
| `vale host-uninstall` | ✅ 已禁用 |
| HTTP 请求 | ✅ 已禁用 |
| NLP Endpoint | ✅ 已禁用 |

### 2. MCP Server

**版本**: 1.0.0  
**位置**: `Docs/mcp/vale-mcp-server/`

#### MCP 工具

| 工具名 | 功能 | 参数 |
|--------|------|------|
| `vale_status` | 检查 Vale 安装状态 | 无 |
| `check_docs` | 检查文档风格问题 | file_path |
| `fix_docs` | 修复文档问题 | file_path, instructions, pattern, replacement |

### 3. Vale 配置生成器

**Skill**: `generating-vale-config`  
**功能**: 将规范文档 (xlsx/md/docx) 转换为 Vale 配置

**使用**:
```
输入: 规范文件 (xlsx/md/docx)
输出: .vale.ini + 规则文件
```

详见 `Docs/skills/generating-vale-config.md`

---

## 安全说明

详见 `Docs/SECURITY.md`

**已修复风险**:

| 风险 | 等级 | 状态 |
|------|------|------|
| 路径遍历漏洞 | 🔴 高 | ✅ 已修复 |
| 符号链接攻击 | 🔴 高 | ✅ 已修复 |
| 正则 DoS | 🟡 中 | ✅ 已修复 |
| 离线依赖缺失 | 🟡 中 | ✅ 已修复 |

**安全评级**: A (适用于内网环境)

**部署前验证**:
```powershell
# 校验 Vale 二进制
Get-FileHash Docs/vale/vale-3.14.1\vale-intranet.exe -Algorithm SHA256
# 应返回: C4641CE9C899F51B5EF50EAF580CE4EFF6CD3040344395C0ED12AFA6AD077414

# 校验 MCP Server 二进制
Get-FileHash Docs\mcp\vale-mcp-server\bin\vale-mcp-server.exe -Algorithm SHA256
# 应返回: 89D8160A1B00149D0866682DA9892AB9C33F808A3C7E37E9D1B89963EB07E72E
```

---

## 开发记录

详见 `Docs/DEVELOPMENT_LOG.md`

---

## 相关资源

- [Vale 官方文档](https://vale.sh/)
- [MCP 协议规范](https://modelcontextprotocol.io/)
- [资料自动化方案](Docs/资料自动化方案v1.md)
