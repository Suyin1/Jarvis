# Vale MCP Server 测试指南

本文档详细介绍 Vale MCP Server 的测试体系，包括单元测试、安全测试、集成测试，以及如何开发和维护测试。

## 目录

1. [测试架构](#测试架构)
2. [运行测试](#运行测试)
3. [测试套件说明](#测试套件说明)
4. [安全测试](#安全测试)
5. [CI/CD 集成](#cicd-集成)
6. [维护指南](#维护指南)

---

## 测试架构

### 测试文件结构

```
Docs/mcp/vale-mcp-server/
├── cmd/server/
│   ├── main.go                 # 本地模式入口
│   ├── main_http.go           # 远程模式入口
│   ├── middleware.go          # 安全中间件
│   └── middleware_test.go     # 中间件测试
│
├── internal/handlers/
│   ├── tools.go              # MCP 工具实现
│   ├── tools_test.go         # 工具测试
│   └── path_test.go          # 路径验证测试
│
├── TESTING.md               # 测试概览（本文档）
└── README.md                # 使用文档
```

### 测试类型

| 类型 | 文件 | 覆盖内容 |
|------|------|---------|
| 单元测试 | `*_test.go` | API Key 认证、路径校验、正则验证 |
| 安全测试 | `*_test.go` | 目录遍历、命令注入、DoS 防护 |
| 集成测试 | 手动 | 端到端功能验证 |

---

## 运行测试

### 本地运行

```powershell
# 进入项目目录
cd Docs/mcp/vale-mcp-server

# 运行所有测试
go test -v ./...

# 运行特定包
go test -v ./cmd/server/...
go test -v ./internal/handlers/...

# 运行单个测试
go test -v -run TestAPIKeyAuth_ValidKey ./cmd/server/

# 查看测试覆盖率
go test -cover ./...
```

### 脚本运行

```powershell
# PowerShell 脚本
cd Docs/mcp\vale-mcp-server

# 运行测试并输出详细结果
go test -v -race -cover ./... | Tee-Object -FilePath test-results.txt

# 检查测试结果
if ($LASTEXITCODE -eq 0) {
    Write-Host "All tests passed!" -ForegroundColor Green
} else {
    Write-Host "Some tests failed!" -ForegroundColor Red
}
```

---

## 测试套件说明

### 1. 中间件测试 (cmd/server/middleware_test.go)

#### API Key 认证测试

| 测试名称 | 测试内容 | 预期结果 |
|---------|---------|---------|
| `TestAPIKeyAuth_ValidKey` | 有效的 API Key | 返回 200 |
| `TestAPIKeyAuth_InvalidKey` | 无效的 API Key | 返回 401 |
| `TestAPIKeyAuth_MissingKey` | 缺少 API Key | 返回 401 |
| `TestAPIKeyAuth_NoAuthRequired` | 未配置 Key（跳过认证） | 返回 200 |
| `TestAPIKeyAuth_KeyFromQuery` | 从 URL 参数传递 Key | 返回 200 |
| `TestAPIKeyAuth_RateLimitExceeded` | 超过速率限制 | 返回 429 |

#### 日志脱敏测试

| 测试名称 | 测试内容 | 预期结果 |
|---------|---------|---------|
| `TestSanitizeLogData_Truncation` | 长日志截断 | 截断至 500 字符 |
| `TestSanitizeLogData_ShortData` | 短日志不变 | 保持原样 |

**运行命令**：
```powershell
go test -v ./cmd/server/...
```

### 2. 路径验证测试 (internal/handlers/path_test.go)

| 测试名称 | 测试内容 | 预期结果 |
|---------|---------|---------|
| `TestValidatePath_ValidPath` | 有效路径 | 返回解析后路径 |
| `TestValidatePath_RelativePath` | 相对路径 | 转换为绝对路径 |
| `TestValidatePath_PathTraversal_ParentDirectory` | `../` 穿越 | 返回错误 |
| `TestValidatePath_PathTraversal_Encoded` | 编码穿越 | 返回错误 |
| `TestValidatePath_NonExistentPath` | 不存在的文件 | 返回路径（无错误） |
| `TestValidatePath_SymlinkOutside` | 指向外部的符号链接 | 返回错误 |
| `TestValidatePath_SymlinkInside` | 目录内的符号链接 | 返回解析后路径 |
| `TestValidatePath_EmptyPath` | 空路径 | 返回错误 |
| `TestValidatePath_ReservedCharacters` | 保留字符 | 拒绝访问 |
| `TestValidatePath_WindowsDriveLetter` | Windows 系统路径 | 返回错误 |
| `TestValidatePath_SameAsAllowedDir` | 允许目录本身 | 允许访问 |

**安全测试覆盖**：
- ✅ 目录遍历攻击防护
- ✅ 符号链接攻击防护
- ✅ Windows 特殊路径防护

### 3. 工具测试 (internal/handlers/tools_test.go)

#### 正则安全测试

| 测试名称 | 测试内容 | 预期结果 |
|---------|---------|---------|
| `TestSafeRegexCompile_ValidPattern` | 有效正则 | 编译成功 |
| `TestSafeRegexCompile_TooLong` | 过长模式（>500字符） | 返回错误 |
| `TestSafeRegexCompile_DangerousPattern_NonCapturing` | 非捕获组 | 警告（允许） |
| `TestSafeRegexCompile_DangerousPattern_Lookahead` | 前瞻断言 | 返回错误 |
| `TestSafeRegexCompile_DangerousPattern_Lookbehind` | 后顾断言 | 返回错误 |
| `TestSafeRegexCompile_DangerousPattern_GreedyStar` | 贪婪量词 | 警告（允许） |
| `TestSafeRegexCompile_InvalidChars` | 无效字符 | 返回错误 |
| `TestSafeRegexCompile_ExcessiveBacktracking` | 回溯风险 | 警告（允许） |
| `TestSafeRegexCompile_SimplePattern` | 简单模式 | 编译成功 |

#### 工具注册测试

| 测试名称 | 测试内容 |
|---------|---------|
| `TestRegisterAllTools` | 注册所有 MCP 工具 |
| `TestValeStatusTool_WithoutVale` | Vale 状态检查 |
| `TestApplyInstructions_Simplify` | 简化指令 |
| `TestApplyInstructions_Passive` | 被动语态检查 |

---

## 安全测试

### 测试覆盖的安全风险

| 风险类型 | 测试方法 | 状态 |
|---------|---------|------|
| 未授权访问 | `TestAPIKeyAuth_*` | ✅ 已测试 |
| 目录遍历 | `TestValidatePath_PathTraversal_*` | ✅ 已测试 |
| 符号链接攻击 | `TestValidatePath_Symlink*` | ✅ 已测试 |
| 命令注入 | 代码审查 + 参数化执行 | ✅ 已实现 |
| 拒绝服务 (DoS) | `TestAPIKeyAuth_RateLimitExceeded` | ✅ 已测试 |
| 正则 DoS | `TestSafeRegexCompile_*` | ✅ 已测试 |
| 日志泄露 | `TestSanitizeLogData_*` | ✅ 已测试 |

### 手动安全测试

```powershell
# 1. 测试目录遍历
$env:VALE_ALLOWED_DIR = "E:\docs"
.\vale-mcp-server-http.exe &
curl -X POST http://localhost:8080/mcp -d '{"file_path": "../secret.txt"}'

# 2. 测试无效 API Key
curl -H "X-API-Key: invalid" http://localhost:8080/mcp

# 3. 测试速率限制
for ($i=0; $i -lt 70; $i++) {
    curl http://localhost:8080/mcp
}
```

---

## CI/CD 集成

### GitHub Actions

创建 `.github/workflows/test.yml`：

```yaml
name: Test

on:
  push:
    paths:
      - 'Docs/mcp/vale-mcp-server/**'
  pull_request:
    paths:
      - 'Docs/mcp/vale-mcp-server/**'

jobs:
  test:
    runs-on: windows-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Go
        uses: actions/setup-go@v5
        with:
          go-version: '1.25'
          
      - name: Download dependencies
        working-directory: Docs/mcp/vale-mcp-server
        run: go mod download
        
      - name: Run tests
        working-directory: Docs/mcp/vale-mcp-server
        run: go test -v -race -coverprofile=coverage.out ./...
        
      - name: Upload coverage
        uses: actions/upload-artifact@v4
        with:
          name: coverage
          path: Docs/mcp/vale-mcp-server/coverage.out
```

### 本地 CI 检查

```powershell
# 在推送前运行所有检查
cd Docs/mcp/vale-mcp-server

# 1. 运行测试
go test -race ./...

# 2. 检查代码格式
go fmt ./...

# 3. 检查 lint（可选）
# go install golang.org/x/lint/golint@latest
# golint -set_exit_status ./...

# 4. 构建二进制
go build -tags http -o bin/vale-mcp-server-http.exe ./cmd/server
go build -o bin/vale-mcp-server.exe ./cmd/server
```

---

## 维护指南

### 添加新测试

#### 步骤 1：创建测试函数

在对应的 `*_test.go` 文件中添加：

```go
func TestNewFeature(t *testing.T) {
    // Arrange
    input := "test input"
    
    // Act
    result := myFunction(input)
    
    // Assert
    if result != expected {
        t.Errorf("Expected %v, got %v", expected, result)
    }
}
```

#### 步骤 2: 运行测试

```powershell
go test -v -run TestNewFeature ./...
```

#### 步骤 3: 更新文档

在本文档中添加测试说明。

### 常见问题

#### Q: 测试失败但代码正确

**检查**：测试用例是否正确设置环境（如 Windows 路径）

#### Q: 符号链接测试跳过

**原因**：Windows 默认禁止普通用户创建符号链接  
**解决**：在开发者模式或以管理员身份运行

#### Q: 测试覆盖率低

**建议**：
1. 增加边界条件测试
2. 增加错误处理测试
3. 增加安全攻击模拟

---

## 测试检查清单

- [ ] 所有单元测试通过
- [ ] 安全测试覆盖所有已知风险
- [ ] 测试文档已更新
- [ ] CI/CD 流程通过
- [ ] 手动安全测试通过

---

## 相关文档

- [MCP Server 使用指南](README.md)
- [远程 MCP 安全分析](../REMOTE_MCP_SECURITY.md)
- [内网部署指南](../INTRANET_DEPLOYMENT.md)

---

**文档版本**：1.0  
**更新时间**：2026-04-09  
**适用版本**：Vale MCP Server 1.0.0+