# Java 参考实现（Apache HttpClient + Fastjson2）

> 基于华为官方示例代码（GetQuickLoginMobilePhoneByCodeDemo）

## 1. 常量定义

```java
package com.example.huawei.constant;

/**
 * 华为一键登录错误码常量
 *
 * resultCode（int）对应关系（来源：华为官方文档）：
 *   60010001 → 系统内部错误，稍后重试
 *   60010002 → 参数不合法，检查入参
 *   60010012 → code 参数不正确（伪造或被篡改）
 *   60010013 → clientSecret 参数不正确
 *   60180003 → code 的 clientId 与入参不一致
 *   60180004 → code 过期（5分钟有效期）
 *   60180005 → code 已被使用过
 *   60180006 → code 授权被取消
 *   60180007 → code 未授权一键登录权限
 *   60180008 → 用户无手机号，展示其他登录方式
 *   60180009 → 手机号信息获取受限（用户地域限制）
 */
public final class HuaweiAccountResultCode {

    private HuaweiAccountResultCode() {}

    // ── 错误码常量 ───────────────────────────────────────────────────────────

    /** 系统内部错误，稍后重试 */
    public static final int SYSTEM_ERROR = 60010001;

    /** 参数不合法，检查入参 */
    public static final int INVALID_PARAM = 60010002;

    /** code 参数不正确（伪造或被篡改）*/
    public static final int INVALID_CODE = 60010012;

    /** clientSecret 参数不正确 */
    public static final int INVALID_CLIENT_SECRET = 60010013;

    /** code 的 clientId 与入参不一致 */
    public static final int CLIENT_ID_MISMATCH = 60180003;

    /** code 过期（5分钟有效期）*/
    public static final int CODE_EXPIRED = 60180004;

    /** code 已被使用过 */
    public static final int CODE_ALREADY_USED = 60180005;

    /** code 授权被取消 */
    public static final int CODE_CANCELLED = 60180006;

    /** code 未授权一键登录权限 */
    public static final int CODE_UNAUTHORIZED = 60180007;

    /** 用户无手机号，展示其他登录方式 */
    public static final int NO_PHONE_NUMBER = 60180008;

    /** 手机号信息获取受限（用户地域限制）*/
    public static final int PHONE_RESTRICTED = 60180009;

    // ── HTTP 状态码常量 ─────────────────────────────────────────────────────

    /** HTTP 请求成功 */
    public static final int HTTP_OK = 200;

    /** HTTP 客户端错误 - 400 */
    public static final int HTTP_STATUS_BAD_REQUEST = 400;

    /** HTTP 认证失败 - 401 */
    public static final int HTTP_STATUS_UNAUTHORIZED = 401;

    /** HTTP 禁止访问 - 403 */
    public static final int HTTP_STATUS_FORBIDDEN = 403;

    /** HTTP 错误网关 - 502 */
    public static final int HTTP_STATUS_BAD_GATEWAY = 502;

}
```

## 2. 依赖（pom.xml，需完整引入）

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

## 3. 配置（application.yml）

### 3.1 配置文件

```yaml
server:
  port: 8080  # 默认端口 8080

huawei:
  oauth:
    quick-login-url: https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
    client-id:     YOUR_CLIENT_ID        # 从 AppGallery Connect 获取
    client-secret: YOUR_CLIENT_SECRET   # 从 AppGallery Connect 获取，严禁硬编码
```

### 3.2 配置说明

| 配置项                            | 说明               | 获取位置                                                                 |
| ------------------------------ | ---------------- | -------------------------------------------------------------------- |
| `huawei.oauth.quick-login-url` | 华为一键登录服务端接口      | 固定值，无需修改                                                             |
| `huawei.oauth.client-id`       | OAuth 2.0 客户端 ID | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID                    |
| `huawei.oauth.client-secret`   | OAuth 2.0 客户端密钥  | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID（凭据）→ Client Secret |

> **⚠️ 运行环境要求：JDK 21**

## 4. 响应 DTO（Fastjson2）

```java
package com.example.huawei.dto;

import com.alibaba.fastjson2.annotation.JSONField;
import lombok.Data;

/**
 * 华为一键登录接口原始响应（内部使用）
 *
 * 接口：POST https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
 * Content-Type: application/json
 *
 * 用于解析华为 API 返回的 JSON 响应，成功时包含用户信息，失败时包含错误码
 */
@Data
public class QuickLoginResponse {

    // ── 成功字段 ────────────────────────────────────────────────────────

    /** 用户 OpenID */
    @JSONField(name = "openId")
    private String openId;

    /** 用户 UnionID */
    @JSONField(name = "unionId")
    private String unionId;

    /** 华为账号绑定号码（含国家码，如 "0086191******08"）*/
    @JSONField(name = "phoneNumber")
    private String phoneNumber;

    /**
     * 手机号实时有效性
     * 0 = 过去90天内无法证明可触达，需进一步验证
     * 1 = 过去90天内可证明可触达，可直接使用
     */
    @JSONField(name = "phoneNumberValid")
    private Integer phoneNumberValid;

    /** 不带国家码的手机号（如 "191******08"）*/
    @JSONField(name = "purePhoneNumber")
    private String purePhoneNumber;

    /** 国际冠码+区号（如 "0086"）*/
    @JSONField(name = "phoneCountryCode")
    private String phoneCountryCode;

    // ── 失败字段 ───────────────────────────────────────────────────────

    /** 错误码，int 类型，成功时此字段不存在或为 0 */
    @JSONField(name = "resultCode")
    private Integer resultCode;

    /** 错误描述 */
    @JSONField(name = "resultDesc")
    private String resultDesc;

    // ── 判断 ────────────────────────────────────────────────────────────

    /**
     * 判断是否成功
     * 成功：HTTP 200 且 resultCode 不存在或为 null/0
     */
    public boolean isSuccess() {
        return resultCode == null || resultCode == 0;
    }
}
```

```java
package com.example.huawei.dto;

import lombok.Data;

/**
 * 用户信息业务对象（业务层使用）
 */
@Data
public class HuaweiUser {
    private String openId;
    private String unionId;
    private String phoneNumber;
    private Integer phoneNumberValid;
    private String purePhoneNumber;
    private String phoneCountryCode;

    /**
     * 从华为 API 响应转换业务对象
     */
    public static HuaweiUser fromQuickLoginResponse(QuickLoginResponse response) {
        if (response == null || !response.isSuccess()) {
            return null;
        }
        HuaweiUser user = new HuaweiUser();
        user.setOpenId(response.getOpenId());
        user.setUnionId(response.getUnionId());
        user.setPhoneNumber(response.getPhoneNumber());
        user.setPhoneNumberValid(response.getPhoneNumberValid());
        user.setPurePhoneNumber(response.getPurePhoneNumber());
        user.setPhoneCountryCode(response.getPhoneCountryCode());
        return user;
    }
}
```

## 5. API 接口请求/响应

```java
package com.example.huawei.dto;

import lombok.Data;

/**
 * 一键登录获取手机号请求DTO
 *
 * 接口：POST /huawei/quickLogin/getPhoneNumber
 */
@Data
public class GetPhoneNumberRequest {

    /** 客户端SDK返回的Authorization Code（必填）*/
    private String code;
}
```

```java
package com.example.huawei.dto;

import lombok.Data;

/**
 * 一键登录成功响应DTO
 *
 * 接口：POST /huawei/quickLogin/getPhoneNumber
 */
@Data
public class GetPhoneNumberResponse {

    /** 用户OpenID */
    private String openId;

    /** 用户UnionID */
    private String unionId;

    /** 华为账号绑定号码（含国家码）*/
    private String phoneNumber;

    /** 手机号实时有效性：0=需验证，1=可直接使用 */
    private Integer phoneNumberValid;

    /** 不带国家码的手机号 */
    private String purePhoneNumber;

    /** 国际冠码+区号 */
    private String phoneCountryCode;

    /** 成功响应工厂方法 */
    public static GetPhoneNumberResponse success(String openId, String unionId, String phoneNumber,
            Integer phoneNumberValid, String purePhoneNumber, String phoneCountryCode) {
        GetPhoneNumberResponse response = new GetPhoneNumberResponse();
        response.setOpenId(openId);
        response.setUnionId(unionId);
        response.setPhoneNumber(phoneNumber);
        response.setPhoneNumberValid(phoneNumberValid);
        response.setPurePhoneNumber(purePhoneNumber);
        response.setPhoneCountryCode(phoneCountryCode);
        return response;
    }
}
```

```java
package com.example.huawei.dto;

import lombok.Data;

/**
 * 一键登录错误响应DTO
 *
 * 接口：POST /huawei/quickLogin/getPhoneNumber
 *
 */
@Data
public class ErrorResponse {

    /** 是否成功 */
    private Boolean success = false;

    /** 错误码 */
    private Integer resultCode;

    /** 错误描述 */
    private String resultDesc;

    /** 业务建议（如展示其他登录方式）*/
    private String suggestion;

    /** 错误响应工厂方法 */
    public static ErrorResponse fail(Integer resultCode, String resultDesc) {
        ErrorResponse response = new ErrorResponse();
        response.setSuccess(false);
        response.setResultCode(resultCode);
        response.setResultDesc(resultDesc);
        return response;
    }

    /** 带建议的错误响应工厂方法 */
    public static ErrorResponse fail(Integer resultCode, String resultDesc, String suggestion) {
        ErrorResponse response = fail(resultCode, resultDesc);
        response.setSuggestion(suggestion);
        return response;
    }
}
```

## 6. 异常类

```java
package com.example.huawei.exception;

import com.example.huawei.constant.HuaweiAccountResultCode;
import lombok.Getter;

/**
 * 华为一键登录服务异常
 *
 * 使用 HuaweiAccountResultCode 中定义的常量判断错误类型
 */
@Getter
public class HuaweiAccountException extends RuntimeException {

    private final Integer resultCode;  // 华为错误码（int），如 60180004
    private final Integer httpStatus;

    public HuaweiAccountException(Integer resultCode, Integer httpStatus, String message) {
        super(message);
        this.resultCode = resultCode;
        this.httpStatus = httpStatus;
    }

    // ── 常用判断 ─────────────────────────────────────────────────────────

    public boolean isCodeExpired()            { return resultCode != null && resultCode == HuaweiAccountResultCode.CODE_EXPIRED; }
    public boolean isCodeAlreadyUsed()        { return resultCode != null && resultCode == HuaweiAccountResultCode.CODE_ALREADY_USED; }
    public boolean isCodeCancelled()          { return resultCode != null && resultCode == HuaweiAccountResultCode.CODE_CANCELLED; }
    public boolean isCodeUnauthorized()       { return resultCode != null && resultCode == HuaweiAccountResultCode.CODE_UNAUTHORIZED; }
    public boolean isNoPhoneNumber()          { return resultCode != null && resultCode == HuaweiAccountResultCode.NO_PHONE_NUMBER; }
    public boolean isPhoneRestricted()        { return resultCode != null && resultCode == HuaweiAccountResultCode.PHONE_RESTRICTED; }
    public boolean isInvalidClientSecret()    { return resultCode != null && resultCode == HuaweiAccountResultCode.INVALID_CLIENT_SECRET; }
    public boolean isClientIdMismatch()       { return resultCode != null && resultCode == HuaweiAccountResultCode.CLIENT_ID_MISMATCH; }
    public boolean isInvalidCode()            { return resultCode != null && resultCode == HuaweiAccountResultCode.INVALID_CODE; }
    public boolean isInvalidParam()           { return resultCode != null && resultCode == HuaweiAccountResultCode.INVALID_PARAM; }
    public boolean isSystemError()            { return resultCode != null && resultCode == HuaweiAccountResultCode.SYSTEM_ERROR; }
}
```

## 7. HTTP 调用工具类（CallUtils）

```java
package com.example.huawei.utils;

import com.alibaba.fastjson2.JSON;
import com.alibaba.fastjson2.JSONObject;
import lombok.extern.slf4j.Slf4j;
import org.apache.http.HttpEntity;
import org.apache.http.HttpStatus;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.config.Registry;
import org.apache.http.config.RegistryBuilder;
import org.apache.http.conn.socket.ConnectionSocketFactory;
import org.apache.http.conn.socket.PlainConnectionSocketFactory;
import org.apache.http.conn.ssl.SSLConnectionSocketFactory;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.impl.conn.PoolingHttpClientConnectionManager;
import org.apache.http.util.EntityUtils;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;
import java.io.IOException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.util.Map;
import java.util.Objects;
import java.util.function.BiFunction;

/**
 * HTTP调用工具类（基于华为官方示例）
 */
@Slf4j
public class CallUtils {

    public static String remoteCallAccountApi(HttpUriRequest request) throws IOException {
        return remoteCall(request, CallUtils::accountApiErrorHandler);
    }

    public static <E extends Exception> String remoteCall(HttpUriRequest request,
            BiFunction<CloseableHttpResponse, String, E> errorHandler) throws IOException, E {
        try (CloseableHttpResponse response = getClient().execute(request)) {
            HttpEntity responseEntity = response.getEntity();
            String ret = responseEntity != null ? EntityUtils.toString(responseEntity) : null;
            EntityUtils.consume(responseEntity);
            if (errorHandler != null) {
                E error = errorHandler.apply(response, ret);
                if (null != error) {
                    throw error;
                }
            }
            return ret;
        }
    }

    public static JSONObject toJsonObject(String json) {
        return JSON.parseObject(json);
    }

    public static StringEntity wrapJsonEntity(Object obj) {
        return new StringEntity(toJsonString(obj), ContentType.create("application/json", "UTF-8"));
    }

    public static String toJsonString(Object obj) {
        return JSON.toJSONString(obj);
    }

    // ── 错误处理器 ─────────────────────────────────────────────────────────

    /**
     * 华为账号 API 错误处理器（关键）
     * 
     * 1. HTTP 状态码 ≠ 200 → 失败
     * 2. HTTP 状态码 = 200 → 解析 resultCode
     *    - resultCode 为 0 或不存在 → 成功
     *    - resultCode 非 0 → 失败
     */
    public static IOException accountApiErrorHandler(CloseableHttpResponse response, String rawBody) {
        int statusCode = response.getStatusLine().getStatusCode();

        // HTTP 状态码不是 200，请求失败
        if (statusCode != HttpStatus.SC_OK) {
            return new IOException("call failed! http status code: " + statusCode + ", response data: " + rawBody);
        }

        // HTTP 状态码为 200，解析响应的 body，判断业务错误码
        JSONObject errorResponseBody = toJsonObject(rawBody);
        Integer resultCode = errorResponseBody.getInteger("resultCode");

        // resultCode 为 0 表示成功，非 0 表示失败
        if (Objects.nonNull(resultCode) && resultCode != 0) {
            return new IOException("call failed! resultCode: " + resultCode + ", response data: " + rawBody);
        }
        return null;
    }

    // ── HTTP Client 配置 ─────────────────────────────────────────────────

    private static CloseableHttpClient getClient() {
        PoolingHttpClientConnectionManager connectionManager = buildConnectionManager(
                new String[]{"TLSv1.2"},
                new String[]{
                        "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
                        "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
                        "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384",
                        "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256"
                }
        );
        connectionManager.setMaxTotal(400);
        connectionManager.setDefaultMaxPerRoute(400);

        RequestConfig config = RequestConfig.custom()
                .setConnectionRequestTimeout(100)
                .setRedirectsEnabled(false)
                .build();

        return HttpClients.custom()
                .useSystemProperties()
                .setConnectionManager(connectionManager)
                .setDefaultRequestConfig(config)
                .build();
    }

    private static PoolingHttpClientConnectionManager buildConnectionManager(String[] supportedProtocols,
            String[] supportedCipherSuites) {
        PoolingHttpClientConnectionManager connectionManager = null;
        try {
            SSLContext sc = SSLContext.getInstance("TLSv1.2");
            TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            tmf.init((KeyStore) null);
            sc.init(null, tmf.getTrustManagers(), null);

            SSLConnectionSocketFactory sslsf = new SSLConnectionSocketFactory(
                    sc,
                    supportedProtocols,
                    supportedCipherSuites,
                    SSLConnectionSocketFactory.getDefaultHostnameVerifier()
            );

            Registry<ConnectionSocketFactory> registry = RegistryBuilder.<ConnectionSocketFactory>create()
                    .register("http", new PlainConnectionSocketFactory())
                    .register("https", sslsf)
                    .build();
            connectionManager = new PoolingHttpClientConnectionManager(registry);
        } catch (NoSuchAlgorithmException | KeyStoreException | KeyManagementException e) {
            log.error("build connect manager failed", e);
        }
        return connectionManager;
    }
}
```

## 8. 核心服务类

```java
package com.example.huawei.service;

import com.alibaba.fastjson2.JSON;
import com.example.huawei.constant.HuaweiAccountResultCode;
import com.example.huawei.dto.HuaweiUser;
import com.example.huawei.dto.QuickLoginResponse;
import com.example.huawei.exception.HuaweiAccountException;
import com.example.huawei.utils.CallUtils;
import lombok.extern.slf4j.Slf4j;
import org.apache.http.client.methods.HttpPost;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

@Slf4j
@Service
public class HuaweiAccountService {

    @Value("${huawei.oauth.quick-login-url}")
    private String quickLoginUrl;

    @Value("${huawei.oauth.client-id}")
    private String clientId;

    @Value("${huawei.oauth.client-secret}")
    private String clientSecret;

    /**
     * 一键登录主流程
     *
     * 接口：POST https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
     * 无需先获取 Access Token，直接调用即可
     *
     * @param authorizationCode 客户端 SDK 返回的 Authorization Code
     * @return 用户信息
     * @throws HuaweiAccountException 业务异常（含错误码）
     */
    public HuaweiUser quickLogin(String authorizationCode) throws HuaweiAccountException {
        try {
            // 调用华为 API，获取原始响应
            QuickLoginResponse response = getQuickLoginMobile(authorizationCode);

            // 检查响应是否成功
            if (!response.isSuccess()) {
                throw new HuaweiAccountException(
                        response.getResultCode(),
                        HuaweiAccountResultCode.HTTP_OK,
                        response.getResultDesc());
            }

            // 转换为业务对象
            return HuaweiUser.fromQuickLoginResponse(response);

        } catch (IOException e) {
            // 解析错误信息
            String msg = e.getMessage();
            Integer resultCode = extractResultCode(msg);
            Integer httpStatus = extractHttpStatus(msg);

            throw new HuaweiAccountException(resultCode, httpStatus, msg);
        }
    }

    /**
     * 调用华为一键登录接口，返回解析后的响应对象
     */
    private QuickLoginResponse getQuickLoginMobile(String authorizationCode) throws IOException {
        HttpPost httpPost = new HttpPost(quickLoginUrl);

        Map<String, Object> reqBody = new HashMap<>();
        reqBody.put("code", authorizationCode);
        reqBody.put("clientId", clientId);
        reqBody.put("clientSecret", clientSecret);

        httpPost.setHeader("Content-Type", "application/json");
        httpPost.setEntity(CallUtils.wrapJsonEntity(reqBody));

        // 使用 accountApiErrorHandler 处理错误
        String rawResponse = CallUtils.remoteCallAccountApi(httpPost);

        // 解析 JSON 为 QuickLoginResponse 对象
        return JSON.parseObject(rawResponse, QuickLoginResponse.class);
    }

    // ── 辅助方法 ─────────────────────────────────────────────────────────

    private Integer extractResultCode(String errorMsg) {
        // 从错误消息中提取 resultCode
        if (errorMsg == null) return null;
        try {
            if (errorMsg.contains("resultCode:")) {
                String[] parts = errorMsg.split("resultCode:");
                if (parts.length > 1) {
                    String codeStr = parts[1].split(",")[0].trim();
                    return Integer.parseInt(codeStr);
                }
            }
        } catch (Exception e) {
            log.warn("Failed to extract resultCode from: {}", errorMsg);
        }
        return null;
    }

    private Integer extractHttpStatus(String errorMsg) {
        if (errorMsg == null) return null;
        try {
            if (errorMsg.contains("http status code:")) {
                String[] parts = errorMsg.split("http status code:");
                if (parts.length > 1) {
                    String statusStr = parts[1].split(",")[0].trim();
                    return Integer.parseInt(statusStr);
                }
            }
        } catch (Exception e) {
            log.warn("Failed to extract httpStatus from: {}", errorMsg);
        }
        return null;
    }
}
```

## 9. Controller

```java
package com.example.huawei.controller;

import com.example.huawei.constant.HuaweiAccountResultCode;
import com.example.huawei.dto.ErrorResponse;
import com.example.huawei.dto.GetPhoneNumberRequest;
import com.example.huawei.dto.GetPhoneNumberResponse;
import com.example.huawei.dto.HuaweiUser;
import com.example.huawei.exception.HuaweiAccountException;
import com.example.huawei.service.HuaweiAccountService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequiredArgsConstructor
public class HuaweiAuthController {

    private final HuaweiAccountService service;

    /**
     * 华为一键登录获取手机号
     *
     * 接口：POST /huawei/quickLogin/getPhoneNumber
     */
    @PostMapping("/huawei/quickLogin/getPhoneNumber")
    public ResponseEntity<?> huaweiQuickLogin(
            @Valid @RequestBody GetPhoneNumberRequest request) {

        String code = request.getCode();
        if (code == null || code.isBlank()) {
            return ResponseEntity.badRequest()
                    .body(ErrorResponse.fail(
                            HuaweiAccountResultCode.INVALID_PARAM,
                            "缺少 Authorization Code"));
        }

        try {
            HuaweiUser user = service.quickLogin(code);

            return ResponseEntity.ok(GetPhoneNumberResponse.success(
                    user.getOpenId(),
                    user.getUnionId(),
                    user.getPhoneNumber(),
                    user.getPhoneNumberValid(),
                    user.getPurePhoneNumber(),
                    user.getPhoneCountryCode()));

        } catch (HuaweiAccountException e) {
            return ResponseEntity.status(mapHttpStatus(e.getResultCode()))
                    .body(handleException(e));
        }
    }

    private ErrorResponse handleException(HuaweiAccountException e) {
        Integer rc = e.getResultCode();
        if (rc == null) rc = HuaweiAccountResultCode.SYSTEM_ERROR;

        // 用户无手机号，业务上展示其他登录方式
        if (e.isNoPhoneNumber()) {
            return ErrorResponse.fail(
                    rc,
                    "用户无手机号",
                    "SHOW_ALTERNATIVE_LOGIN");
        }

        String msg = switch (rc) {
            case HuaweiAccountResultCode.SYSTEM_ERROR -> "系统内部错误";
            case HuaweiAccountResultCode.INVALID_PARAM -> "参数不合法";
            case HuaweiAccountResultCode.INVALID_CODE -> "code 参数不正确";
            case HuaweiAccountResultCode.INVALID_CLIENT_SECRET -> "clientSecret 参数不正确";
            case HuaweiAccountResultCode.CLIENT_ID_MISMATCH -> "code 中的 clientId 和入参不一致";
            case HuaweiAccountResultCode.CODE_EXPIRED -> "code 过期";
            case HuaweiAccountResultCode.CODE_ALREADY_USED -> "code 已经被使用过";
            case HuaweiAccountResultCode.CODE_CANCELLED -> "code 授权被取消";
            case HuaweiAccountResultCode.CODE_UNAUTHORIZED -> "code 未授权华为账号一键登录权限";
            case HuaweiAccountResultCode.PHONE_RESTRICTED -> "手机号信息获取受限";
            default -> e.getMessage();
        };

        return ErrorResponse.fail(rc, msg);
    }

    private int mapHttpStatus(Integer rc) {
        if (rc == null) return HuaweiAccountResultCode.HTTP_STATUS_BAD_GATEWAY;
        return switch (rc) {
            case HuaweiAccountResultCode.SYSTEM_ERROR -> HuaweiAccountResultCode.HTTP_STATUS_BAD_GATEWAY;
            case HuaweiAccountResultCode.INVALID_PARAM, HuaweiAccountResultCode.INVALID_CODE,
                 HuaweiAccountResultCode.CLIENT_ID_MISMATCH, HuaweiAccountResultCode.CODE_EXPIRED,
                 HuaweiAccountResultCode.CODE_ALREADY_USED, HuaweiAccountResultCode.CODE_CANCELLED,
                 HuaweiAccountResultCode.CODE_UNAUTHORIZED -> HuaweiAccountResultCode.HTTP_STATUS_BAD_REQUEST;
            case HuaweiAccountResultCode.INVALID_CLIENT_SECRET -> HuaweiAccountResultCode.HTTP_STATUS_UNAUTHORIZED;
            case HuaweiAccountResultCode.PHONE_RESTRICTED -> HuaweiAccountResultCode.HTTP_STATUS_FORBIDDEN;
            default -> HuaweiAccountResultCode.HTTP_STATUS_BAD_GATEWAY;
        };
    }
}
```
