# 更新日志 (Changelog)

## [开发中] v0.1.0

### 日期
2026-04-09

### 功能开发

- **新增 vale_status 工具**
  - 检查 Vale CLI 是否正确安装
  - 返回 Vale 版本信息
  - 验证 Vale 可执行文件路径

- **新增 check_docs 工具**
  - 使用 Vale 检查文档风格/格式/语法问题
  - 支持自定义检查规则
  - 返回问题列表及位置信息

- **新增 fix_docs 工具**
  - 根据用户指令自动修复文档问题
  - 支持被动语态、冗余词汇等常见问题修复

### 项目结构

```
vale-mcp-server/
├── cmd/server/main.go           # 服务入口
├── internal/handlers/tools.go   # MCP 工具实现
├── config/
│   ├── .vale.ini               # Vale 配置
│   └── styles/                 # 自定义规则
│       ├── Passive.yml         # 被动语态检测
│       ├── Simplicity.yml      # 简洁性检测
│       └── WeaselWords.yml     # 冗余词汇检测
├── go.mod                       # Go 模块定义
├── README.md                    # 项目文档
└── CHANGELOG.md                 # 本文件
```

### 技术实现

- **传输协议**: stdio（安全，零网络请求）
- **MCP SDK**: github.com/modelcontextprotocol/go-sdk v1.4.1
- **配置**: Vale 配置存储在 config/ 目录

### 待完成

- [ ] 测试服务运行
- [ ] 推送到 GitHub

### 已完成

- [x] 运行 go mod tidy 下载依赖
- [x] 编译 vale-mcp-server.exe
- [x] 编译内网安全版 Vale (vale-intranet.exe) - 使用 MSYS2 + gcc 编译成功

### Vale 内网安全版

已创建基于 Vale v3.14.1 的**内网安全版**，禁用所有网络功能：

- `vale sync` - 已禁用
- `vale install` - 已禁用
- `vale host-install/host-uninstall` - 已禁用
- `NLPEndpoint` 配置 - 已禁用

**编译产物位置：**
- Vale: `E:\ai\Jarvis\Docs\vale\vale-3.14.1\vale-intranet.exe` (需要 gcc)
- MCP Server: `E:\ai\Jarvis\Docs\mcp\vale-mcp-server\vale-mcp-server.exe`

**编译要求：**
Vale 内网安全版需要 CGO 支持（gcc 编译器）。在 Windows 上可以安装 MinGW-w64 或 MSYS2 来提供 gcc。

---

## 历史版本

（暂无）
