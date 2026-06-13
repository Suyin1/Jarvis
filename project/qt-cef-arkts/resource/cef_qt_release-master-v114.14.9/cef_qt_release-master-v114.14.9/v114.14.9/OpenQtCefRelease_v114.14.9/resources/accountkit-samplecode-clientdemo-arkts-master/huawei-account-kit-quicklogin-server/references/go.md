# Go 参考实现

> 参考 Java 版实现思路，Go 语言等效代码。来源文件：`references/java.md`

## 代码说明

本实现遵循 Go 语言编码规范，提供完整的注释说明：

- 每个公开函数和结构体都有详细的文档注释
- 关键逻辑块包含行内注释说明
- 错误处理遵循 Go 语言的惯用模式

## 1. 安装依赖

```bash
go get github.com/google/uuid
# 标准库 net/http、encoding/json、bytes、fmt、crypto/tls 已内置
```

## 2. 常量与配置

```go
package huawei

import (
    "os"
    "strconv"

    "gopkg.in/yaml.v3"
)

// ── 错误码常量 ───────────────────────────────────────────────────────────

// 华为账号 API 错误码
const (
    // 错误码常量
    ErrCodeSystemError         = 60010001 // 系统内部错误，稍后重试
    ErrCodeInvalidParam        = 60010002 // 参数不合法，检查入参
    ErrCodeInvalidCode         = 60010012 // code 参数不正确（伪造或被篡改）
    ErrCodeInvalidClientSecret = 60010013 // clientSecret 参数不正确
    ErrCodeClientIdMismatch    = 60180003 // code 的 clientId 与入参不一致
    ErrCodeCodeExpired         = 60180004 // code 过期（5分钟有效期）
    ErrCodeCodeAlreadyUsed     = 60180005 // code 已被使用过
    ErrCodeCodeCancelled       = 60180006 // code 授权被取消
    ErrCodeCodeUnauthorized    = 60180007 // code 未授权一键登录权限
    ErrCodeNoPhoneNumber       = 60180008 // 用户无手机号，展示其他登录方式
    ErrCodePhoneRestricted     = 60180009 // 手机号信息获取受限（用户地域限制）

    // HTTP 状态码
    HTTPStatusOK           = 200 // HTTP 请求成功
    HTTPStatusBadRequest   = 400 // HTTP 客户端错误
    HTTPStatusUnauthorized = 401 // HTTP 认证失败
    HTTPStatusForbidden    = 403 // HTTP 禁止访问
    HTTPStatusBadGateway   = 502 // HTTP 错误网关

    // 业务常量
    SuccessCode         = 0     // 成功响应码
)

// ── 配置常量 ───────────────────────────────────────────────────────────

const (
    // 默认华为一键登录服务端接口
    DefaultQuickLoginURL = "https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber"
)

// ── 配置结构 ───────────────────────────────────────────────────────────

// Config 应用配置
type Config struct {
    ClientID      string
    ClientSecret  string
    QuickLoginURL string
    ServerPort    int
}

// 配置读取（优先配置文件，备选环境变量）
func LoadConfig(configPath string) (Config, error) {
    // 1. 尝试从配置文件读取
    if configPath != "" {
        data, err := os.ReadFile(configPath)
        if err == nil {
            var yamlCfg struct {
                Huawei struct {
                    Oauth struct {
                        QuickLoginURL string `yaml:"quick-login-url"`
                        ClientID      string `yaml:"client-id"`
                        ClientSecret  string `yaml:"client-secret"`
                    } `yaml:"oauth"`
                } `yaml:"huawei"`
                Server struct {
                    Port int `yaml:"port"`
                } `yaml:"server"`
            }
            if err := yaml.Unmarshal(data, &yamlCfg); err == nil && yamlCfg.Huawei.Oauth.ClientID != "" {
                port := yamlCfg.Server.Port
                if port == 0 {
                    port = 8080
                }
                url := yamlCfg.Huawei.Oauth.QuickLoginURL
                if url == "" {
                    url = DefaultQuickLoginURL
                }
                return Config{
                    ClientID:      yamlCfg.Huawei.Oauth.ClientID,
                    ClientSecret:  yamlCfg.Huawei.Oauth.ClientSecret,
                    QuickLoginURL: url,
                    ServerPort:    port,
                }, nil
            }
        }
    }

    // 2. 回退到环境变量
    getEnv := func(key, fallback string) string {
        if v := os.Getenv(key); v != "" {
            return v
        }
        return fallback
    }
    getEnvInt := func(key string, fallback int) int {
        if v := os.Getenv(key); v != "" {
            if port, err := strconv.Atoi(v); err == nil {
                return port
            }
        }
        return fallback
    }

    cfg := Config{
        ClientID:      getEnv("HUAWEI_CLIENT_ID", ""),
        ClientSecret:  getEnv("HUAWEI_CLIENT_SECRET", ""),
        QuickLoginURL: DefaultQuickLoginURL,
        ServerPort:    getEnvInt("SERVER_PORT", 8080),
    }

    // 验证必要配置
    if cfg.ClientID == "" || cfg.ClientSecret == "" {
        return cfg, &ConfigError{Message: "缺少必要配置"}
    }
    return cfg, nil
}

// 配置错误
type ConfigError struct {
    Message string
}

func (e *ConfigError) Error() string {
    return e.Message
}
```

## 2.1 配置文件（推荐）

### config.yaml

```yaml
huawei:
  oauth:
    quick-login-url: https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
    client-id: YOUR_CLIENT_ID        # 从 AppGallery Connect 获取
    client-secret: YOUR_CLIENT_SECRET # 从 AppGallery Connect 获取，严禁硬编码

server:
  port: 8080
```

### 配置说明

| 配置项                            | 说明               | 获取位置                                                                 |
| ------------------------------ | ---------------- | -------------------------------------------------------------------- |
| `huawei.oauth.quick-login-url` | 华为一键登录服务端接口      | 固定值，无需修改                                                             |
| `huawei.oauth.client-id`       | OAuth 2.0 客户端 ID | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID                    |
| `huawei.oauth.client-secret`   | OAuth 2.0 客户端密钥  | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID（凭据）→ Client Secret |
| `server.port`                  | 服务端口（可选，默认 8080） | -                                                                    |

### 安装 YAML 解析库

```bash
go get gopkg.in/yaml.v3
```

## 2.2 环境变量（备选，用于容器化等场景）

如无法使用配置文件，可通过环境变量配置：

| 环境变量                   | 说明               | 获取位置                                                                 |
| ---------------------- | ---------------- | -------------------------------------------------------------------- |
| `HUAWEI_CLIENT_ID`     | OAuth 2.0 客户端 ID | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID                    |
| `HUAWEI_CLIENT_SECRET` | OAuth 2.0 客户端密钥  | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID（凭据）→ Client Secret |
| `SERVER_PORT`          | 服务端口（可选，默认 8080） | -                                                                    |

### 配置示例

```bash
# Linux/Mac
export HUAWEI_CLIENT_ID=your_client_id
export HUAWEI_CLIENT_SECRET=your_client_secret
export SERVER_PORT=8080

# Windows CMD
set HUAWEI_CLIENT_ID=your_client_id
set HUAWEI_CLIENT_SECRET=your_client_secret
set SERVER_PORT=8080
```

## 3. 数据结构

```go
// ── 请求体 ─────────────────────────────────────────────────────────────────

type QuickLoginRequest struct {
    Code         string `json:"code"`          // Authorization Code
    ClientID    string `json:"clientId"`       // OAuth 2.0 客户端 ID
    ClientSecret string `json:"clientSecret"`  // 密钥
}

// ── 成功响应 ────────────────────────────────────────────────────────────────

type QuickLoginResponse struct {
    OpenID           string `json:"openId"`            // 用户 OpenID
    UnionID          string `json:"unionId"`          // 用户 UnionID
    PhoneNumber      string `json:"phoneNumber"`     // 含国家码手机号，如 "0086191******08"
    PhoneNumberValid int    `json:"phoneNumberValid"` // 0=需验证，1=可直接使用
    PurePhoneNumber  string `json:"purePhoneNumber"` // 不带国家码，如 "191******08"
    PhoneCountryCode string `json:"phoneCountryCode"` // 国际冠码+区号，如 "0086"
    // 失败字段（成功时不存在）
    ResultCode int    `json:"resultCode"` // 错误码（int），成功时字段不存在于 JSON 中，Go 解码为 0
    ResultDesc string `json:"resultDesc"` // 错误描述
}

// IsSuccess 是否成功：resultCode 为 0 或字段不存在即成功
func (r QuickLoginResponse) IsSuccess() bool {
    return r.ResultCode == 0
}

// HuaweiUser 用户信息业务对象
type HuaweiUser struct {
    OpenID           string
    UnionID          string
    PhoneNumber      string
    PhoneNumberValid int
    PurePhoneNumber  string
    PhoneCountryCode string
}

// ── API 接口请求/响应 ────────────────────────────────────────

// GetPhoneNumberRequest 一键登录获取手机号请求DTO
type GetPhoneNumberRequest struct {
    Code string `json:"code"` // 客户端SDK返回的Authorization Code（必填）
}

// GetPhoneNumberResponse 一键登录获取手机号响应DTO
type GetPhoneNumberResponse struct {
    Success          bool   `json:"success"`           // 是否成功
    ResultCode       int    `json:"resultCode"`        // 错误码，0表示成功
    ResultDesc       string `json:"resultDesc"`        // 错误描述
    OpenID           string `json:"openId,omitempty"`  // 用户OpenID
    UnionID          string `json:"unionId,omitempty"` // 用户UnionID
    PhoneNumber      string `json:"phoneNumber,omitempty"`      // 华为账号绑定号码（含国家码）
    PhoneNumberValid *int   `json:"phoneNumberValid,omitempty"` // 手机号实时有效性：0=需验证，1=可直接使用
    PurePhoneNumber  string `json:"purePhoneNumber,omitempty"`  // 不带国家码的手机号
    PhoneCountryCode string `json:"phoneCountryCode,omitempty"` // 国际冠码+区号
    Suggestion       string `json:"suggestion,omitempty"`       // 业务建议（如展示其他登录方式）
}

// NewGetPhoneNumberResponseSuccess 成功响应工厂方法
func NewGetPhoneNumberResponseSuccess(user *HuaweiUser) *GetPhoneNumberResponse {
    resp := &GetPhoneNumberResponse{
        Success:          true,
        ResultCode:       SuccessCode,
        OpenID:           user.OpenID,
        UnionID:          user.UnionID,
        PhoneNumber:      user.PhoneNumber,
        PurePhoneNumber:  user.PurePhoneNumber,
        PhoneCountryCode: user.PhoneCountryCode,
    }
    resp.PhoneNumberValid = &user.PhoneNumberValid
    return resp
}

// NewGetPhoneNumberResponseFail 失败响应工厂方法
func NewGetPhoneNumberResponseFail(resultCode int, resultDesc string) *GetPhoneNumberResponse {
    return &GetPhoneNumberResponse{
        Success:    false,
        ResultCode: resultCode,
        ResultDesc: resultDesc,
    }
}

// NewGetPhoneNumberResponseFailWithSuggestion 带建议的失败响应工厂方法
func NewGetPhoneNumberResponseFailWithSuggestion(resultCode int, resultDesc, suggestion string) *GetPhoneNumberResponse {
    resp := NewGetPhoneNumberResponseFail(resultCode, resultDesc)
    resp.Suggestion = suggestion
    return resp
}
```

## 4. 异常定义（对齐官方 accountApiErrorHandler 逻辑）

```go
package huawei

import "fmt"

/**
 * 华为一键登录服务异常
 *
 * 官方错误处理逻辑（accountApiErrorHandler）：
 *   1. HTTP 状态码 ≠ 200 → 失败
 *   2. HTTP 状态码 = 200 → 解析 resultCode
 *        - resultCode 为 0 或字段不存在 → 成功
 *        - resultCode 非 0 → 失败
 *
 * 使用常量定义错误码，参见常量定义章节
 */
type AccountError struct {
    ResultCode int    // 华为 resultCode（int），如 60180004
    HTTPStatus int    // HTTP 状态码
    Message    string
}

func (e *AccountError) Error() string {
    return fmt.Sprintf("[%d] %s", e.ResultCode, e.Message)
}

// ── 常用判断 ──────────────────────────────────────────────────────────────

func (e *AccountError) IsCodeExpired()         bool { return e.ResultCode == ErrCodeCodeExpired }
func (e *AccountError) IsCodeAlreadyUsed()     bool { return e.ResultCode == ErrCodeCodeAlreadyUsed }
func (e *AccountError) IsCodeCancelled()       bool { return e.ResultCode == ErrCodeCodeCancelled }
func (e *AccountError) IsCodeUnauthorized()    bool { return e.ResultCode == ErrCodeCodeUnauthorized }
func (e *AccountError) IsNoPhoneNumber()       bool { return e.ResultCode == ErrCodeNoPhoneNumber }
func (e *AccountError) IsPhoneRestricted()     bool { return e.ResultCode == ErrCodePhoneRestricted }
func (e *AccountError) IsInvalidClientSecret() bool { return e.ResultCode == ErrCodeInvalidClientSecret }
func (e *AccountError) IsClientIdMismatch()    bool { return e.ResultCode == ErrCodeClientIdMismatch }
func (e *AccountError) IsInvalidCode()         bool { return e.ResultCode == ErrCodeInvalidCode }
func (e *AccountError) IsInvalidParam()        bool { return e.ResultCode == ErrCodeInvalidParam }
func (e *AccountError) IsSystemError()         bool { return e.ResultCode == ErrCodeSystemError }
```

## 5. 核心客户端

```go
package huawei

import (
    "bytes"
    "crypto/tls"
    "encoding/json"
    "fmt"
    "io"
    "net/http"
    "time"
)

// Client 华为 Account Kit 一键登录客户端
//
// 使用连接池复用技术，避免频繁创建连接：
// - 维护长连接（Keep-Alive），减少 TCP 握手开销
// - 控制并发连接数，避免资源耗尽
// - 配置 TLS 安全参数
type Client struct {
    cfg  Config        // 应用配置
    http *http.Client  // HTTP 客户端（带连接池）
}

// NewClient 创建华为账号客户端
//
// 使用连接池配置：
// - MaxIdleConns: 最大空闲连接数
// - MaxConnsPerHost: 每个主机最大并发连接数
// - TLS 1.2+ 安全配置
//
// Args:
//   cfg: 应用配置（包含 clientId, clientSecret 等）
//
// Returns:
//   *Client: 配置好的客户端实例
func NewClient(cfg Config) *Client {
    // TLS 配置：强制 TLS 1.2+，使用安全的加密套件
    tlsConfig := &tls.Config{
        MinVersion: tls.VersionTLS12,
        CipherSuites: []uint16{
            tls.TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            tls.TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            tls.TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            tls.TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
        },
    }

    // HTTP 传输层配置：连接池参数
    tr := &http.Transport{
        TLSClientConfig: tlsConfig,
        MaxIdleConns:    400,      // 最大空闲连接数
        MaxConnsPerHost: 400,      // 每个主机最大并发连接数
    }

    return &Client{
        cfg:  cfg,
        http: &http.Client{
            Transport: tr,
            Timeout:   10 * time.Second,  // 请求超时时间
        },
    }
}

/**
 * 一键登录主流程（对齐官方 accountApiErrorHandler 逻辑）
 *
 * 接口：POST https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
 * Content-Type: application/json
 * 无需先获取 Access Token，直接调用即可
 *
 * 错误处理规则（华为官方 CallUtils.accountApiErrorHandler）：
 *   1. HTTP 状态码 ≠ 200 → 失败
 *   2. HTTP 状态码 = 200 → 解析 resultCode
 *        - resultCode 为 0 或不存在 → 成功
 *        - resultCode 非 0 → 失败
 *
 * Args:
 *   code: 客户端 SDK 返回的 Authorization Code
 *
 * Returns:
 *   *HuaweiUser: 用户信息对象
 *   error: 错误（如果请求失败）
 */
func (c *Client) QuickLogin(code string) (*HuaweiUser, error) {
    // 构建请求体
    reqBody := QuickLoginRequest{
        Code:         code,
        ClientID:     c.cfg.ClientID,
        ClientSecret: c.cfg.ClientSecret,
    }

    // 序列化为 JSON
    jsonBody, err := json.Marshal(reqBody)
    if err != nil {
        return nil, &AccountError{ResultCode: ErrCodeInvalidParam, Message: "构建请求体失败: " + err.Error()}
    }

    // 创建 HTTP POST 请求
    req, err := http.NewRequest(http.MethodPost, c.cfg.QuickLoginURL, bytes.NewReader(jsonBody))
    if err != nil {
        return nil, &AccountError{ResultCode: ErrCodeSystemError, Message: "创建请求失败: " + err.Error()}
    }
    req.Header.Set("Content-Type", "application/json")

    // 发送请求（使用连接池中的可用连接）
    resp, err := c.http.Do(req)
    if err != nil {
        return nil, &AccountError{ResultCode: ErrCodeSystemError, HTTPStatus: 0,
            Message: "网络请求失败: " + err.Error()}
    }
    defer resp.Body.Close()

    // 读取响应体
    body, _ := io.ReadAll(resp.Body)
    bodyStr := string(body)

    // ── 华为官方 accountApiErrorHandler 逻辑 ─────────────────────────
    // HTTP 状态码 ≠ 200 → 失败
    if resp.StatusCode != HTTPStatusOK {
        return nil, &AccountError{
            ResultCode: ErrCodeInvalidParam,
            HTTPStatus: resp.StatusCode,
            Message:    fmt.Sprintf("call failed! http status code: %d, response data: %s", resp.StatusCode, bodyStr),
        }
    }

    // HTTP = 200，解析 resultCode
    var data QuickLoginResponse
    if err := json.Unmarshal(body, &data); err != nil {
        return nil, &AccountError{
            ResultCode: ErrCodeSystemError,
            HTTPStatus: resp.StatusCode,
            Message:    "解析响应失败: " + err.Error(),
        }
    }

    // resultCode 非 0 → 失败
    if !data.IsSuccess() {
        return nil, &AccountError{
            ResultCode: data.ResultCode,
            HTTPStatus: resp.StatusCode,
            Message:    data.ResultDesc,
        }
    }

    // 成功返回用户信息
    return &HuaweiUser{
        OpenID:           data.OpenID,
        UnionID:          data.UnionID,
        PhoneNumber:      data.PhoneNumber,
        PhoneNumberValid: data.PhoneNumberValid,
        PurePhoneNumber:  data.PurePhoneNumber,
        PhoneCountryCode: data.PhoneCountryCode,
    }, nil
}
```

## 6. HTTP Handler 示例

```go
package handler

import (
    "encoding/json"
    "fmt"
    "net/http"

    "your-module/huawei"
)

// HuaweiCallback 华为一键登录获取手机号
// 接口：POST /huawei/quickLogin/getPhoneNumber
func HuaweiCallback(w http.ResponseWriter, r *http.Request) {
    if r.Method != http.MethodPost {
        http.Error(w, "只支持 POST", http.StatusMethodNotAllowed)
        return
    }

    // 解析请求
    var req huawei.GetPhoneNumberRequest
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil || req.Code == "" {
        resp := huawei.NewGetPhoneNumberResponseFail(
            huawei.ErrCodeInvalidParam,
            "缺少 Authorization Code")
        writeResponse(w, http.StatusBadRequest, resp)
        return
    }

    // 从配置文件或环境变量加载配置
    cfg, err := huawei.LoadConfig("config.yaml")
    if err != nil {
        resp := huawei.NewGetPhoneNumberResponseFail(
            huawei.ErrCodeInvalidParam,
            err.Error())
        writeResponse(w, http.StatusBadRequest, resp)
        return
    }
    client := huawei.NewClient(cfg)

    user, err := client.QuickLogin(req.Code)
    if err != nil {
        writeError(w, err)
        return
    }

    // 返回成功响应
    resp := huawei.NewGetPhoneNumberResponseSuccess(user)
    writeResponse(w, http.StatusOK, resp)
}

func writeResponse(w http.ResponseWriter, status int, resp *huawei.GetPhoneNumberResponse) {
    w.Header().Set("Content-Type", "application/json")
    w.WriteHeader(status)
    json.NewEncoder(w).Encode(resp)
}

func writeError(w http.ResponseWriter, err error) {
    ae, ok := err.(*huawei.AccountError)
    if !ok {
        resp := huawei.NewGetPhoneNumberResponseFail(
            huawei.ErrCodeSystemError,
            fmt.Sprintf("%v", err))
        writeResponse(w, http.StatusInternalServerError, resp)
        return
    }

    status := huawei.HTTPStatusBadGateway
    msg := "系统内部错误"

    switch {
    case ae.IsNoPhoneNumber():
        resp := huawei.NewGetPhoneNumberResponseFailWithSuggestion(
            ae.ResultCode,
            "用户无手机号",
            "SHOW_ALTERNATIVE_LOGIN")
        writeResponse(w, http.StatusOK, resp)
        return
    case ae.IsSystemError():
        status = huawei.HTTPStatusBadGateway
        msg = "系统内部错误"
    case ae.IsInvalidParam():
        status = huawei.HTTPStatusBadRequest
        msg = "参数不合法"
    case ae.IsInvalidCode():
        status = huawei.HTTPStatusBadRequest
        msg = "code 参数不正确"
    case ae.IsInvalidClientSecret():
        status = huawei.HTTPStatusUnauthorized
        msg = "clientSecret 参数不正确"
    case ae.IsClientIdMismatch():
        status = huawei.HTTPStatusBadRequest
        msg = "code 中的 clientId 和入参不一致"
    case ae.IsCodeExpired():
        status = huawei.HTTPStatusBadRequest
        msg = "code 过期"
    case ae.IsCodeAlreadyUsed():
        status = huawei.HTTPStatusBadRequest
        msg = "code 已经被使用过"
    case ae.IsCodeCancelled():
        status = huawei.HTTPStatusBadRequest
        msg = "code 授权被取消"
    case ae.IsCodeUnauthorized():
        status = huawei.HTTPStatusBadRequest
        msg = "code 未授权华为账号一键登录权限"
    case ae.IsPhoneRestricted():
        status = huawei.HTTPStatusForbidden
        msg = "手机号信息获取受限"
    }

    resp := huawei.NewGetPhoneNumberResponseFail(ae.ResultCode, msg)
    writeResponse(w, status, resp)
}
```

## 7. Gin 框架示例（可选）

```go
package router

import (
    "github.com/gin-gonic/gin"
    "your-module/handler"
)

func SetupAuthRouter(r *gin.Engine) {
    // 接口：POST /huawei/quickLogin/getPhoneNumber
    // 默认端口 8080
    r.POST("/huawei/quickLogin/getPhoneNumber", handler.HuaweiCallback)
}
```

## 8. 启动服务示例

```go
package main

import (
    "log"
    "net/http"
    "your-module/handler"
)

func main() {
    http.HandleFunc("/huawei/quickLogin/getPhoneNumber", handler.HuaweiCallback)
    // 默认端口 8080
    log.Println("服务器启动，监听端口 8080")
    log.Fatal(http.ListenAndServe(":8080", nil))
}
```
