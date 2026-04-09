# Jarvis 项目开发记录

## 文档信息

| 项目 | 内容 |
|------|------|
| 项目名称 | Jarvis 文档规范检查系统 |
| 开始日期 | 2026-04-03 |
| 当前版本 | 1.0.0 |

---

## 开发历程

### 2026-04-09 - 阶段三：功能验证与安全修复

#### 本日工作

| 任务 | 状态 | 说明 |
|------|------|------|
| Vale 内网版功能验证 | ✅ 完成 | 版本检查、规则检查、sync 禁用验证 |
| MCP Server 编译 | ✅ 完成 | API 适配 v1.5.0，编译成功 |
| 安全漏洞修复 | ✅ 完成 | R-001/R-002/R-005 全部修复 |
| MCP Server vendor | ✅ 完成 | vendor 目录已创建 |

#### 安全修复详情

| 风险ID | 修复方式 |
|--------|----------|
| R-001 | 添加 `validatePath()` 函数，验证路径在 allowedDir 内，使用 `filepath.Clean()` 阻止 `../` 攻击 |
| R-002 | 使用 `os.Lstat()` 检测符号链接，`filepath.EvalSymlinks()` 解析后验证目标路径 |
| R-005 | 添加 `safeRegexCompile()` 函数，限制 pattern 长度(500)、检测危险构造、测试回溯 |

#### MCP Server 源码结构

```
vale-mcp-server/
├── cmd/server/main.go       # 入口 (更新为 v1.5.0 API)
├── internal/handlers/tools.go  # 工具实现 (含安全修复)
├── config/                  # Vale 配置
├── vendor/                  # Go 依赖 (离线可用)
├── bin/vale-mcp-server.exe  # 编译二进制
└── go.mod                   # 依赖声明
```

#### 验证结果

**Vale 内网版测试**:
```bash
# 版本检查
$ vale-intranet.exe --version
vale version master

# sync 禁用验证
$ vale-intranet.exe sync
[INTRANET-SAFE] sync command is disabled.

# 文档检查
$ vale-intranet.exe test.md
✔ 0 errors, 0 warnings and 0 suggestions in 1 file.
```

**MCP Server 状态**:
- 源码完整: `cmd/server/main.go`, `internal/handlers/tools.go`
- 配置文件: `.vale.ini`, `styles/`
- 待办: 需要 Go 环境编译

#### 安全漏洞识别

| 风险ID | 名称 | 等级 | 状态 |
|--------|------|------|------|
| R-001 | 路径遍历漏洞 | 🔴 高 | 待修复 |
| R-002 | 符号链接攻击 | 🔴 高 | 待修复 |
| R-003 | Vale 二进制注入 | 🟡 中 | 待观察 |
| R-004 | 配置注入 | 🟡 中 | 待观察 |
| R-005 | 正则表达式 DoS | 🟡 中 | 待修复 |
| R-008 | MCP Server 依赖缺失 | 🟡 中 | 待处理 |

#### 新增文档

| 文件 | 说明 |
|------|------|
| `Docs/VALE_CONFIG_GUIDE.md` | Vale 配置生成指南 |
| `Docs/skills/generating-vale-config.md` | 配置生成 Skill |
| `Docs/SECURITY.md` | 安全性分析报告 |
| `Docs/README.md` | 项目总说明 |

---

### 2026-04-06 - 阶段二：内网安全改造

#### 工作内容

| 任务 | 状态 |
|------|------|
| Vale 源码迁移 | ✅ 完成 |
| 禁用网络功能 | ✅ 完成 |
| vendor 依赖打包 | ✅ 完成 |
| MCP Server 迁移 | ✅ 完成 |

#### 安全修改点

| 文件 | 修改内容 |
|------|----------|
| `cmd/vale/command.go:88` | sync 命令返回错误 |
| `cmd/vale/api.go:64` | install 命令返回错误 |
| `cmd/vale/native.go:238` | host-install 返回错误 |
| `cmd/vale/native.go:245` | host-uninstall 返回错误 |
| `cmd/vale/util.go:43` | HTTP 请求返回错误 |
| `internal/nlp/http.go:21` | NLP API 禁用 |
| `internal/core/config.go:222` | NLPEndpoint 配置禁用 |

---

### 2026-04-03 - 阶段一：需求分析与方案设计

#### 产出文档

| 文档 | 说明 |
|------|------|
| `Docs/资料自动化方案v1.md` | 完整的资料开发自动化方案 |
| `Docs/dp-资料开发效率提升方案.md` | 效率提升方案 |

#### Skill 设计

| Skill | 说明 | 状态 |
|-------|------|------|
| checking-docs-guide-quality | 开发指南质量检查 | 设计中 |
| checking-sample-code-quality | 示例代码质量检查 | 设计中 |
| checking-version-consistency | 版本一致性检查 | 设计中 |
| generating-vale-config | Vale 配置生成 | 完成 |
| generating-doc-section | 文档章节生成 | 设计中 |
| generating-sample-code | 示例代码生成 | 设计中 |

---

## 技术架构

```
Jarvis/
├── Docs/
│   ├── vale/
│   │   ├── vale-3.14.1/          # Vale 源码 (内网版)
│   │   │   ├── cmd/vale/         # CLI 入口
│   │   │   ├── internal/         # 核心库
│   │   │   ├── vendor/          # Go 依赖 (241模块)
│   │   │   └── vale-intranet.exe # 编译二进制
│   │   │
│   │   └── VALE_CONFIG_GUIDE.md  # 配置指南
│   │
│   ├── mcp/
│   │   └── vale-mcp-server/     # MCP Server
│   │       ├── cmd/server/      # 入口
│   │       ├── internal/handlers/ # 工具实现
│   │       └── config/          # Vale 配置
│   │
│   ├── skills/
│   │   ├── generating-vale-config.md # 配置生成 Skill
│   │   └── checking-docs-guide-quality.md # (待开发)
│   │
│   ├── README.md                # 项目总览
│   ├── SECURITY.md              # 安全分析
│   └── 资料自动化方案v1.md       # 需求方案
│
└── (其他项目文件)
```

---

## 待办事项

### 高优先级

| 任务 | 说明 | 状态 |
|------|------|------|
| 修复 R-001 | 路径遍历防护 | ✅ 已完成 |
| 修复 R-002 | 符号链接检测 | ✅ 已完成 |
| 编译 MCP Server | Go 环境编译 | ✅ 已完成 |
| 功能测试套件 | 单元测试 | 待开始 |

### 中优先级

| 任务 | 说明 | 状态 |
|------|------|------|
| 修复 R-005 | 正则表达式 DoS 防护 | ✅ 已完成 |
| MCP Server vendor | 添加 vendor 目录 | ✅ 已完成 |
| Vale 配置测试 | 验证配置生成 | ✅ 已完成 |

### 低优先级

| 任务 | 说明 | 状态 |
|------|------|------|
| MCP Server 二进制 | 编译后分发 | ✅ 已完成 |
| 集成测试 | 端到端测试 | ✅ 已完成 |

---

## 常用命令

### Vale 内网版

```bash
# 版本检查
./vale-intranet.exe --version

# 检查文档
./vale-intranet.exe --config=.vale.ini file.md

# 检查目录
./vale-intranet.exe --config=.vale.ini docs/

# JSON 输出
./vale-intranet.exe --config=.vale.ini --output=JSON file.md
```

### MCP Server (需要 Go 环境)

```bash
# 编译
cd Docs/mcp/vale-mcp-server
go build -o vale-mcp-server.exe ./cmd/server

# 运行
./vale-mcp-server

# 指定允许目录
VALE_ALLOWED_DIR=/path/to/docs ./vale-mcp-server
```

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2026-04-09 | 初始版本，包含 Vale 内网版、MCP Server、配置生成 Skill |
