# 远程 MCP 安全分析报告

本文档分析远程 MCP 架构的安全风险，并提供内网环境下的安全加固建议。

## 目录

1. [威胁模型](#威胁模型)
2. [安全需求分析](#安全需求分析)
3. [数据流安全分析](#数据流安全分析)
4. [安全风险与缓解措施](#安全风险与缓解措施)
5. [部署安全配置](#部署安全配置)
6. [安全检查清单](#安全检查清单)

---

## 威胁模型

### 资产保护

| 资产 | 敏感性 | 说明 |
|------|--------|------|
| 用户文档内容 | 高 | 文档可能包含敏感信息（代码、配置、业务数据） |
| MCP 请求/响应 | 高 | 包含文件名、文件内容片段、检查结果 |
| Vale 规则配置 | 中 | 内部编写的检查规则可能被泄露 |
| MCP Server 访问日志 | 中 | 可推断用户工作模式和文档结构 |

### 威胁来源

| 威胁来源 | 可能性 | 影响 |
|----------|--------|------|
| 外部攻击者 | 低 | 内网隔离，外部无法直接访问 |
| 内部未授权用户 | 中 | 需要防止越权访问 |
| 客户端漏洞 | 中 | AI 客户端可能被攻击 |
| 中间人攻击 | 低 | 内网环境相对可信 |
| 日志泄露 | 中 | 日志包含敏感信息 |

---

## 安全需求分析

### 核心安全目标

1. **机密性（Confidentiality）**：文档内容在传输和存储过程中不被未授权访问
2. **完整性（Integrity）**：请求和响应数据不被篡改
3. **可用性（Availability）**：服务稳定运行，防止 DoS
4. **身份验证（Authentication）**：确认客户端身份
5. **授权控制（Authorization）**：控制用户可访问的目录和功能

### 内网特殊考虑

- 内网 ≠ 完全安全，仍需纵深防御
- 防止"内部威胁"（离职员工、误操作）
- 最小权限原则
- 日志审计追溯

---

## 数据流安全分析

### 数据流图

```
┌──────────────┐    HTTPS     ┌─────────────────┐    subprocess    ┌─────────────┐
│  AI 客户端    │◄────────────►│  MCP HTTP Server │◄──────────────►│  Vale CLI  │
│ (Claude/     │   请求/响应   │  (Go)            │  文件路径/内容   │  (检查引擎) │
│  Cursor等)   │              │                 │                 │             │
└──────────────┘              └─────────────────┘                 └─────────────┘
       │                             │                                  │
       │                             ▼                                  │
       │                    ┌─────────────────┐                         │
       │                    │  访问控制层      │                         │
       │                    │  - 认证          │                         │
       │                    │  - 授权          │                         │
       │                    │  - 路径限制      │                         │
       │                    └─────────────────┘                         │
       │                             │                                  │
       ▼                             ▼                                  │
┌──────────────┐              ┌─────────────────┐                      │
│  用户浏览器   │              │  日志/审计系统    │                      │
│  (可选前端)   │              │  (访问记录)      │                      │
└──────────────┘              └─────────────────┘                      │
```

### 关键数据流

| 阶段 | 数据 | 风险点 |
|------|------|--------|
| 1. 客户端请求 | `{"method": "tools/call", "params": {"name": "lintFile", "arguments": {...}}}` | 未授权访问、请求篡改 |
| 2. 身份验证 | API Key / Token | 凭证泄露、暴力破解 |
| 3. 路径校验 | 文件路径 `"/data/project/README.md"` | 目录遍历攻击 |
| 4. 读取文件 | 文件内容 | 读取敏感文件、符号链接攻击 |
| 5. 调用 Vale | 命令行参数 | 命令注入 |
| 6. 返回结果 | 检查结果 JSON | 响应篡改、日志泄露 |

---

## 安全风险与缓解措施

### 风险 1：未授权访问

**风险描述**：未经认证的客户端访问 MCP Server

**缓解措施**：

```go
// 实现 API Key 认证
func authenticateRequest(req *http.Request) error {
    apiKey := req.Header.Get("X-API-Key")
    if apiKey == "" {
        return errors.New("missing API key")
    }
    validKeys := getValidKeys() // 从环境变量或配置文件加载
    if !contains(validKeys, apiKey) {
        return errors.New("invalid API key")
    }
    return nil
}

// 中间件包装
func authMiddleware(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if err := authenticateRequest(r); err != nil {
            http.Error(w, err.Error(), http.StatusUnauthorized)
            return
        }
        next.ServeHTTP(w, r)
    })
}
```

**部署配置**：

```bash
# 环境变量配置
export MCP_API_KEY="your-secure-random-key-min-32-chars"
export MCP_ALLOWED_DIR="/data/docs"
```

---

### 风险 2：目录遍历攻击

**风险描述**：攻击者通过路径穿越访问禁止目录

**缓解措施**：

```go
func validatePath(baseDir, requestPath string) error {
    // 解析并规范化路径
    absBase, _ := filepath.Abs(baseDir)
    absReq, _ := filepath.Abs(requestPath)
    
    // 检查是否在允许目录内
    if !strings.HasPrefix(absReq, absBase) {
        return errors.New("access denied: path outside allowed directory")
    }
    
    // 检查符号链接
    realPath, _ := filepath.EvalSymlinks(absReq)
    realBase, _ := filepath.EvalSymlinks(absBase)
    if !strings.HasPrefix(realPath, realBase) {
        return errors.New("access denied: symlink points outside allowed directory")
    }
    
    return nil
}
```

---

### 风险 3：命令注入

**风险描述**：恶意构造文件名导致命令注入

**缓解措施**：

```go
func safeExecuteVale(filePath string) (string, error) {
    // 严格校验文件名格式
    if containsShellChars(filePath) {
        return "", errors.New("invalid filename: contains dangerous characters")
    }
    
    // 使用参数而非 shell 命令
    cmd := exec.Command("vale", "lint", "--no-exit", filePath)
    // 不使用 shell=true，避免注入
    
    output, err := cmd.Output()
    // ... 处理结果
}

func containsShellChars(s string) bool {
    dangerous := []string{"&", "|", ";", "`", "$", "(", ")", "<", ">", "\n", "\r"}
    for _, c := range dangerous {
        if strings.Contains(s, c) {
            return true
        }
    }
    return false
}
```

---

### 风险 4：日志泄露

**风险描述**：日志记录敏感信息（文档内容、文件路径）

**缓解措施**：

```go
func sanitizeForLog(data string) string {
    // 移除或脱敏敏感字段
    re := regexp.MustCompile(`"(content|filePath)":\s*"[^"]*"`)
    return re.ReplaceAllString(data, `"$1": "[REDACTED]"`)
}

func logRequest(req *http.Request, logger *log.Logger) {
    logger.Printf("method=%s path=%s user=%s", 
        req.Method, 
        req.URL.Path,
        getUserID(req), // 不记录具体文件路径
    )
}
```

**日志配置建议**：

```yaml
# 日志配置
logging:
  level: info
  sensitive_fields:
    - content
    - filePath
    - arguments
  max_length: 200  # 截断过长日志
```

---

### 风险 5：DoS 攻击

**风险描述**：大量请求导致服务不可用

**缓解措施**：

```go
// 请求速率限制
import "golang.org/x/time/rate"

func rateLimitMiddleware(next http.Handler) http.Handler {
    limiter := rate.NewLimiter(rate.Limit(10), 20) // 10 req/s, burst 20
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if !limiter.Allow() {
            http.Error(w, "rate limit exceeded", http.StatusTooManyRequests)
            return
        }
        next.ServeHTTP(w, r)
    })
}

// 请求大小限制
const maxRequestSize = 1 * 1024 * 1024 // 1MB
```

---

### 风险 6：凭证泄露

**风险描述**：API Key 在代码、配置或日志中泄露

**缓解措施**：

1. **环境变量存储**：不硬编码 Key，使用环境变量
2. **配置加密**：敏感配置使用密钥加密存储
3. **Key 轮换**：定期更换 API Key
4. **最小权限**：每个客户端使用独立 Key，便于吊销

```bash
# 正确示例
export MCP_API_KEY="prod-key-xxxxx"

# 错误示例（禁止）
# API_KEY="hardcoded-key"  # 不应在代码中硬编码
```

---

## 部署安全配置

### 1. 网络层

```nginx
# Nginx 反向代理配置（推荐）
server {
    listen 443 ssl;
    server_name mcp.internal.company.com;
    
    # SSL 配置
    ssl_certificate /etc/nginx/ssl/server.crt;
    ssl_certificate_key /etc/nginx/ssl/server.key;
    ssl_protocols TLSv1.2 TLSv1.3;
    
    # API Key 验证
    location /mcp {
        proxy_pass http://localhost:8080;
        
        # 验证请求头中的 API Key
        if ($http_x_api_key = "") {
            return 401 "Missing API Key";
        }
        
        # 转发必要头
        proxy_set_header X-API-Key $http_x_api_key;
    }
}
```

### 2. 服务层

```bash
# 启动 MCP Server（安全配置）
export MCP_API_KEY="your-secure-key-min-32-chars"
export MCP_ALLOWED_DIR="/data/company/docs"
export MCP_MAX_REQUEST_SIZE=1048576
export MCP_RATE_LIMIT=10

# 绑定本地回环接口（不暴露到公网）
./vale-mcp-server --host 127.0.0.1 --port 8080
```

### 3. 客户端配置

```json
{
  "mcpServers": {
    "vale-remote": {
      "url": "https://mcp.internal.company.com/mcp",
      "env": {
        "X-API-Key": "${MCP_API_KEY}"
      }
    }
  }
}
```

---

## 安全检查清单

### 部署前检查

- [ ] API Key 长度 ≥ 32 字符，使用随机生成
- [ ] MCP_ALLOWED_DIR 指向正确目录，无软链接
- [ ] 绑定地址为 127.0.0.1，不暴露到外网
- [ ] 启用 HTTPS（生产环境）
- [ ] 日志中已脱敏敏感字段

### 运行时监控

- [ ] 监控异常请求（路径遍历尝试）
- [ ] 监控大文件请求（可能为数据泄露）
- [ ] 监控高频请求（DoS 攻击）
- [ ] 定期审计日志

### 定期维护

- [ ] 轮换 API Key（建议每月）
- [ ] 更新依赖库（安全补丁）
- [ ] 审查访问日志
- [ ] 更新安全配置

---

## 总结

| 安全目标 | 措施 |
|----------|------|
| 身份验证 | API Key 认证 |
| 访问控制 | VALE_ALLOWED_DIR 目录限制 |
| 输入校验 | 路径规范化、文件名白名单 |
| 命令安全 | 参数化调用，无 shell 执行 |
| 日志安全 | 脱敏处理，限制长度 |
| 传输安全 | HTTPS 加密（生产环境） |
| 可用性 | 速率限制、请求大小限制 |

**核心原则**：内网环境仍需纵深防御，多层安全机制确保单一防线失效不会导致全面沦陷。

---

## 附录：配置示例

### 完整的安全启动脚本

```bash
#!/bin/bash
# mcp-server-secure.sh

# 环境变量（从安全存储加载）
export MCP_API_KEY="${MCP_API_KEY}"  # 从 Vault 或环境注入
export MCP_ALLOWED_DIR="/data/docs"
export MCP_MAX_REQUEST_SIZE=1048576
export MCP_RATE_LIMIT=20
export MCP_LOG_LEVEL=warn

# 启动服务（仅监听本地）
cd /opt/vale-mcp-server
./vale-mcp-server \
    --host 127.0.0.1 \
    --port 8080 \
    --auth required \
    --log-file /var/log/mcp-server.log
```

### AI 客户端安全配置

```json
{
  "mcpServers": {
    "vale-intranet": {
      "command": "vale-mcp-server",
      "args": ["--stdio"],
      "env": {
        "VALE_ALLOWED_DIR": "/data/docs"
      }
    },
    "vale-remote": {
      "url": "https://mcp.internal.company.com/mcp",
      "headers": {
        "X-API-Key": "${MCP_API_KEY}"
      }
    }
  }
}
```

> 注意：本地 MCP 模式（stdio）不需要网络传输，安全性更高，但需要用户本地安装。远程 MCP 模式提供更好的集中管理，但需要严格的安全配置。

---

**文档版本**：1.0  
**更新时间**：2026-04-09  
**适用版本**：Vale MCP Server 1.0.0+