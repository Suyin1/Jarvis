---
name: huawei-account-kit-quicklogin-server
description: |
  华为账号一键登录服务端开发助手。生成Java/Go/PHP/Python代码，处理接口报错（如60180007/60180004/60180005）、getPhoneNumber接口调用、权限申请等。
  触发场景（Use when user mentions）：
  - 华为账号一键登录的服务端代码生成（Java/Go/PHP/Python任一语言）
  - 华为一键登录相关错误码（6001开头或6018开头的resultCode）
  - 华为一键登录接口报错/调用失败/权限问题
  - getPhoneNumber接口相关问题
  **重要约束**：
  1. 生成的代码包含可运行的单元测试
  2. 所有字符串/数字常量必须抽取到常量类中定义
  3.**如果用户未指定服务端语言，先从当前项目识别需要的服务端语言，无法识别服务端语言时询问服务端语言**：Java/Python/PHP/Golang 
  4.**根据用户指定的语言从reference目录中读取对应语言的md文件，参照其中的代码生成**
  5. **【强制】代码规范检查：使用强类型DTO而非Map、Object等弱类型**
---

# 华为 Account Kit 一键登录 · 服务端开发助手

## ⚠️ 重要前置说明

> **在使用本skill生成代码前，请确保已完成华为账号一键登录权限申请**
> 
> 权限申请参考：https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/account-config-permissions

> 华为开发者文档API参考：`account-api-get-user-info-quicklogin-by-code`（https://developer.huawei.com/consumer/cn/doc/harmonyos-references/account-api-get-user-info-quicklogin-by-code）

**服务端只需调用一个接口，无需换取 Access Token。**

**涉及一个华为REST API调用：**` https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber`

```
客户端 SDK
  → 用户授权 → SDK 返回 Authorization Code
  → 客户端将 Code 提交到应用服务端

应用服务端
  → 只需一次 POST 调用
  → https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
  → 直接返回 openId / unionId / phoneNumber
```

---

## 一、核心概念

| 字段                              | 说明                                               |
| ------------------------------- | ------------------------------------------------ |
| `Authorization Code`（`code`）    | SDK 返回的一次性授权码，有效期 **5 分钟**，只能使用一次                |
| `OpenID`                        | 应用维度用户唯一标识。不同应用 / 元服务的 OpenID 不同                 |
| `UnionID`                       | 开发者维度唯一标识。同开发者账号下多个应用共享同一个 UnionID               |
| `Client ID`（`clientId`）         | AppGallery Connect「项目设置 → 应用 → OAuth 2.0 客户端 ID」 |
| `Client Secret`（`clientSecret`） | AppGallery Connect 生成的密钥，**严禁暴露在前端**             |

---

## 二、接口说明

### 接口信息

```
POST https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
Content-Type: application/json
```

### 请求参数

**Request Header**

| 参数             | 必选  | 说明                     |
| -------------- | --- | ---------------------- |
| `Content-Type` | ✅   | 固定值：`application/json` |

**Request Body（JSON）**

| 参数             | 必选  | 类型     | 说明                                    |
| -------------- | --- | ------ | ------------------------------------- |
| `code`         | ✅   | String | 客户端 SDK 返回的 Authorization Code        |
| `clientId`     | ✅   | String | AppGallery Connect 的 OAuth 2.0 客户端 ID |
| `clientSecret` | ✅   | String | AppGallery Connect 的密钥                |

**请求示例**

```http
POST /oauth2/v6/quickLogin/getPhoneNumber HTTP/1.1
Host: account-api.cloud.huawei.com
Content-Type: application/json

{"code":"<code>","clientId":"<clientId>","clientSecret":"<clientSecret>"}
```

### 响应参数

**成功时（HTTP 200 + resultCode 不存在）**

| 参数                 | 类型      | 说明                                 |
| ------------------ | ------- | ---------------------------------- |
| `openId`           | String  | 用户 OpenID                          |
| `unionId`          | String  | 用户 UnionID                         |
| `phoneNumber`      | String  | 华为账号绑定号码（含国家码，如 `0086191******08`） |
| `phoneNumberValid` | Integer | 手机号实时有效性：`0`=需验证，`1`=可直接使用         |
| `purePhoneNumber`  | String  | 不带国家码的手机号（如 `191******08`）         |
| `phoneCountryCode` | String  | 国际冠码+区号（如 `0086`）                  |

**响应示例（成功）**

```json
{
  "openId": "AQAxrBzThFv*****lv9tV_4rMCc",
  "unionId": "AQAxrB1HNA*****n-IfWRSUVq2M7xU",
  "phoneNumber": "0086191******08",
  "phoneNumberValid": 1,
  "purePhoneNumber": "191******08",
  "phoneCountryCode": "0086"
}
```

**失败时（HTTP 200 + resultCode 存在）**

```json
{
  "resultCode": 60180008,
  "resultDesc": "user or phone number not exist"
}
```

---

## 三、错误码参考

> **来源**：华为官方文档 `account-api-get-user-info-quicklogin-by-code`
> 
> **更多错误码详解请参考**：`https://developer.huawei.com/consumer/cn/doc/harmonyos-references/account-api-get-user-info-quicklogin-by-code`

### 3.1 HTTP 响应码

| HTTP 状态码 | 描述                                                             | 解决方法                                           |
| -------- | -------------------------------------------------------------- | ---------------------------------------------- |
| **200**  | 仅表示本次接口调用成功，**实际业务结果需通过 Response Body 中的 `resultCode`（int）判断** | —                                              |
| **400**  | 参数错误                                                           | 请根据文档排查请求参数是否符合规范                              |
| **403**  | 无权限访问                                                          | 通常是调用方网络安全策略阻止了访问，请检查网络环境配置。若仍无法解决，请通过在线提单提交问题 |
| **404**  | 找不到服务                                                          | 请检查请求 URI 是否正确                                 |
| **405**  | 不支持的 http 请求 method                                            | 请检查 http 请求 method 是否与接口说明一致                   |
| **500**  | 服务内部错误                                                         | 请通过在线提单提交问题                                    |
| **502**  | 请求连接异常，常见于网络状况不稳定                                              | 建议稍后重试，若仍无法解决，请通过在线提单提交问题                      |
| **503**  | 系统流控                                                           | 触发系统流控，请稍后重试                                   |
| **504**  | 请求连接超时，常见于网络状况不稳定                                              | 建议稍后重试，若仍无法解决，请通过在线提单提交问题                      |
| **590**  | 服务内部错误                                                         | 请通过在线提单提交问题                                    |

### 3.2 业务错误码（`resultCode`，int 类型）

| 错误码        | 描述                          | 解决方法                                                                                |
| ---------- | --------------------------- | ----------------------------------------------------------------------------------- |
| `60010001` | 系统内部错误                      | 请稍后重试，若仍无法解决，请通过在线提单提交问题                                                            |
| `60010002` | 参数不合法                       | 请按照错误描述及接口 Request Body 参数说明检查入参                                                    |
| `60010012` | `code` 参数不正确                | `code` 参数传值不正确，可能原因：伪造的无效 code 或 code 被篡改                                           |
| `60010013` | `clientSecret` 参数不正确        | `clientSecret` 参数传值不正确，请核对 AppGallery Connect「应用基本信息」中的 Client Secret               |
| `60180003` | `code` 中的 `clientId` 和入参不一致 | `code` 获取时的 clientId 与当前接口参数 clientId 不一致，请检查入参 `clientId` 是否与 AGC 配置的 Client ID 一致 |
| `60180004` | `code` 过期                   | `code` 只有 **5 分钟**有效期，超过有效期后将无法继续使用。请引导用户重新授权，获取新的 code 再重试                         |
| `60180005` | `code` 已经被使用过               | `code` 只能用一次，请重新获取 code 再重试                                                         |
| `60180006` | `code` 授权被取消                | 用户取消授权，导致 code 失效，请重新获取 code 再重试                                                    |
| `60180007` | `code` 未授权华为账号一键登录权限        | 可能原因：① 应用未完成华为账号一键登录权限申请；② code 不是通过华为账号的一键登录组件获取到的                                 |
| `60180008` | 用户无手机号                      | 用户华为账号未绑定手机号，该异常场景应用需要展示**其他登录方式**                                                  |
| `60180009` | 手机号信息获取受限                   | 华为账号一键登录服务**仅对中国境内**（香港特别行政区、澳门特别行政区、中国台湾除外）用户提供                                    |

### 3.3 错误码处理决策树

```
收到响应
  │
  ├─ HTTP ≠ 200
  │    ├─ 400 → 参数错误，检查 code / clientId / clientSecret
  │    ├─ 403 → 无权限访问，检查网络环境或提单
  │    ├─ 404 → URI 错误，确认接口地址
  │    ├─ 502 → 网络不稳定，稍后重试
  │    ├─ 503 → 流控限速，稍后重试
  │    └─ 500/504/590 → 服务异常，提单处理
  │
  └─ HTTP = 200
       ├─ 无 resultCode 字段 → 成功，解析用户信息
       └─ 有 resultCode（int）
            ├─ 60010001 → 系统内部错误，稍后重试或提单
            ├─ 60010002 → 参数不合法，检查入参
            ├─ 60010012 → code 被篡改或伪造，引导用户重新登录
            ├─ 60010013 → clientSecret 不正确，检查 AGC 配置
            ├─ 60180003 → code 的 clientId 与当前入参不一致，检查 clientId
            ├─ 60180004 → code 过期（5分钟），引导用户重新授权
            ├─ 60180005 → code 已使用过，重新获取 code
            ├─ 60180006 → 用户取消授权，重新获取 code
            ├─ 60180007 → code 未授权一键登录权限，① 检查一键登录权限申请 ② 检查code是否通过华为账号的一键登录组件获取
            ├─ 60180008 → 用户无手机号，展示其他登录方式
             └─ 60180009 → 服务受限，确认用户地域（中国境内，香港特别行政区、澳门特别行政区、中国台湾除外）
```

---

## 四、开发注意事项

### 4.1 常见错误速查

| 现象                    | 最可能原因                 | 解决方案                                                                                                                                                     |
| --------------------- | --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `resultCode=60010013` | `clientSecret` 传值错误   | 核对 AGC「应用基本信息 → OAuth 2.0 客户端 ID（凭据）→ Client Secret」                                                                                                     |
| `resultCode=60180003` | `clientId` 与 code 不匹配 | 核对 AGC 配置的 Client ID，确认与客户端授权时使用的一致                                                                                                                      |
| `resultCode=60180004` | code 过期（超过 5 分钟）      | 重新获取 code（有效期仅 5 分钟）                                                                                                                                     |
| `resultCode=60180005` | code 重复使用             | 每次登录必须重新获取 code                                                                                                                                          |
| `resultCode=60180006` | 用户取消授权                | 引导用户重新授权                                                                                                                                                 |
| `resultCode=60180007` | 未授权一键登录权限             | ① 确认 AGC 已申请「华为账号一键登录」权限  <br/>② 确认code来源是否为华为账号的一键登录组件；<br/>详情可参考华为开发者指南：`https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/account-faq-21` |
| `resultCode=60180008` | 用户未绑定手机号              | 展示其他登录方式（如短信验证码）                                                                                                                                         |
| `resultCode=60180009` | 手机号信息获取受限             | 华为账号一键登录服务仅对中国境内（香港特别行政区、澳门特别行政区、中国台湾除外）用户提供                                                                                                             |

### 4.2 安全注意事项

```
⚠️ Client Secret 严禁硬编码在前端，必须存储在服务端环境变量或密钥管理服务
⚠️ 生产环境必须使用 HTTPS
⚠️ Authorization Code 有效期仅 5 分钟，且只能使用一次
⚠️ Authorization Code 必须通过华为账号的一键登录组件获取
⚠️ 获取手机号的用户必须是中国境内用户（香港特别行政区、澳门特别行政区、中国台湾除外）
⚠️ phoneNumberValid=0 时需额外验证手机号有效性
```

---

## 五、启动配置（重要提醒）

> **⚠️ 启动服务前必须配置以下凭证，否则服务将无法正常运行**

### ClientId 和 ClientSecret 配置

| 配置项             | 说明               | 获取位置                                                                 |
| --------------- | ---------------- | -------------------------------------------------------------------- |
| `client-id`     | OAuth 2.0 客户端 ID | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID                    |
| `client-secret` | OAuth 2.0 客户端密钥  | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID（凭据）→ Client Secret |

### 各语言配置说明

> **详细配置步骤请参考对应语言的 md 文件**：

| 语言         | 首选配置方式             | 备选配置方式 | 环境变量                                       |
| ---------- | ------------------ | ------ | ------------------------------------------ |
| **Java**   | `application.yml`  | -      | -                                          |
| **Go**     | 配置文件 `config.yaml` | 环境变量   | `HUAWEI_CLIENT_ID`, `HUAWEI_CLIENT_SECRET` |
| **PHP**    | 配置文件 `config.yaml` | 环境变量   | `HUAWEI_CLIENT_ID`, `HUAWEI_CLIENT_SECRET` |
| **Python** | 配置文件 `config.yaml` | 环境变量   | `HUAWEI_CLIENT_ID`, `HUAWEI_CLIENT_SECRET` |

> **安全提示**：`client-secret` 严禁硬编码在代码中，建议使用环境变量或密钥管理服务（如 AWS Secrets Manager、阿里云 KMS 等）存储。

---

## 六、参考代码（必须严格遵循）

> **重要**：生成代码时必须严格参考 references 目录下对应语言的 md 文件，包括：
> 
> - 完整的 pom 依赖（版本号必须一致）
> - 代码结构和命名规范
> - 常量定义方式
> - 错误处理逻辑
> - 单元测试要求

各语言完整实现请参考：

| 语言             | 文件路径                   |
| -------------- | ---------------------- |
| **Java（参考实现）** | `references/java.md`   |
| **Go**         | `references/go.md`     |
| **PHP**        | `references/php.md`    |
| **Python**     | `references/python.md` |

### Java 生成要求（重要）

> **⚠️ 重要提醒**：以下依赖版本号为参考版本，代码生成后请根据实际项目需要替换为合适的版本号

生成 Java 代码时，**pom.xml 依赖必须完整引用 java.md 中的以下依赖**：

```xml
<dependencies>
    <dependency>
        <artifactId>fastjson2</artifactId>
        <groupId>com.alibaba.fastjson2</groupId>
        <version>2.0.51</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
    <dependency>
        <artifactId>httpclient</artifactId>
        <groupId>org.apache.httpcomponents</groupId>
        <version>4.5.6</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
    <dependency>
        <artifactId>lombok</artifactId>
        <groupId>org.projectlombok</groupId>
        <version>1.18.32</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
    <dependency>
        <artifactId>logback-classic</artifactId>
        <groupId>ch.qos.logback</groupId>
        <version>1.5.12</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
    <dependency>
        <artifactId>logback-core</artifactId>
        <groupId>ch.qos.logback</groupId>
        <version>1.5.12</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
    <dependency>
        <artifactId>logback-access</artifactId>
        <groupId>ch.qos.logback</groupId>
        <version>1.5.12</version><!-- 请替换为项目需要的版本号 -->
    </dependency>
</dependencies>
```

---

## 七、代码生成执行完成提醒

> **⚠️ 代码生成完成后，必须提醒用户：生成代码中引用的三方件版本号（如 fastjson2、httpclient、lombok、logback 等）为参考版本，请替换为项目实际使用的版本号**
