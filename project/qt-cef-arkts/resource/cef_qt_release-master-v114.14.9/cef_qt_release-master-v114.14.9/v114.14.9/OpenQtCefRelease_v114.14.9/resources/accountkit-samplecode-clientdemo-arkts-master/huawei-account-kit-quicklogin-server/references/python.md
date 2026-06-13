# Python 参考实现

> 参考 Java 版实现思路，Python 语言等效代码。来源文件：`references/java.md`
> 依赖：`pip install httpx pyyaml`

## 性能优化说明

为提升接口响应速度，本实现采用以下优化策略：

1. **连接池复用**：使用全局单例 HTTP 客户端，避免每次请求创建新连接
2. **Keep-Alive**：启用 HTTP Keep-Alive 保持 TCP 连接复用
3. **连接预建**：启动时预建立连接，减少请求等待时间
4. **HTTP/2 支持**：如服务器支持，自动启用 HTTP/2 加速通信

## 1. 配置文件（推荐）

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

## 2. 环境变量（备选，用于容器化等场景）

如无法使用配置文件，可通过环境变量配置：

| 环境变量                   | 说明               | 获取位置                                                                 |
| ---------------------- | ---------------- | -------------------------------------------------------------------- |
| `HUAWEI_CLIENT_ID`     | OAuth 2.0 客户端 ID | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID                    |
| `HUAWEI_CLIENT_SECRET` | OAuth 2.0 客户端密钥  | AppGallery Connect → 项目设置 → 应用 → OAuth 2.0 客户端 ID（凭据）→ Client Secret |

### 配置示例

```bash
# 设置环境变量（Linux/Mac）
export HUAWEI_CLIENT_ID=your_client_id
export HUAWEI_CLIENT_SECRET=your_client_secret

# Windows CMD
set HUAWEI_CLIENT_ID=your_client_id
set HUAWEI_CLIENT_SECRET=your_client_secret
```

## 3. 安装依赖

```bash
pip install httpx pyyaml
```

## 4. 模块说明

代码按以下文件名组织（Flask/FastAPI 示例需要这些文件）：

- `huawei_constants.py` - 常量定义（第5节）
- `huawei_dto.py` - 数据结构（第7节）
- `huawei_client.py` - 核心客户端（第8节）
- `app.py` - Flask 接口示例（第9节）或 FastAPI 接口示例（第10节）

## 5. 常量定义

```python
from typing import Final

# ── 错误码常量 ─────────────────────────────────────────────────────────

# 华为账号 API 错误码
SYSTEM_ERROR: Final[int] = 60010001          # 系统内部错误，稍后重试
INVALID_PARAM: Final[int] = 60010002         # 参数不合法，检查入参
INVALID_CODE: Final[int] = 60010012          # code 参数不正确（伪造或被篡改）
INVALID_CLIENT_SECRET: Final[int] = 60010013 # clientSecret 参数不正确
CLIENT_ID_MISMATCH: Final[int] = 60180003    # code 的 clientId 与入参不一致
CODE_EXPIRED: Final[int] = 60180004           # code 过期（5分钟有效期）
CODE_ALREADY_USED: Final[int] = 60180005      # code 已被使用过
CODE_CANCELLED: Final[int] = 60180006         # code 授权被取消
CODE_UNAUTHORIZED: Final[int] = 60180007     # code 未授权一键登录权限
NO_PHONE_NUMBER: Final[int] = 60180008       # 用户无手机号，展示其他登录方式
PHONE_RESTRICTED: Final[int] = 60180009      # 手机号信息获取受限（用户地域限制）

# ── HTTP 状态码常量 ───────────────────────────────────────────────────

HTTP_OK: Final[int] = 200                    # HTTP 请求成功
HTTP_STATUS_BAD_REQUEST: Final[int] = 400    # HTTP 客户端错误
HTTP_STATUS_UNAUTHORIZED: Final[int] = 401   # HTTP 认证失败
HTTP_STATUS_FORBIDDEN: Final[int] = 403      # HTTP 禁止访问
HTTP_STATUS_BAD_GATEWAY: Final[int] = 502    # HTTP 错误网关

# ── 业务常量 ─────────────────────────────────────────────────────────

SUCCESS_CODE: Final[int] = 0                 # 成功响应码

# 错误码到 HTTP 状态码的映射
RESULT_CODE_TO_HTTP_STATUS: dict[int, int] = {
    SYSTEM_ERROR: HTTP_STATUS_BAD_GATEWAY,
    INVALID_PARAM: HTTP_STATUS_BAD_REQUEST,
    INVALID_CODE: HTTP_STATUS_BAD_REQUEST,
    INVALID_CLIENT_SECRET: HTTP_STATUS_UNAUTHORIZED,
    CLIENT_ID_MISMATCH: HTTP_STATUS_BAD_REQUEST,
    CODE_EXPIRED: HTTP_STATUS_BAD_REQUEST,
    CODE_ALREADY_USED: HTTP_STATUS_BAD_REQUEST,
    CODE_CANCELLED: HTTP_STATUS_BAD_REQUEST,
    CODE_UNAUTHORIZED: HTTP_STATUS_BAD_REQUEST,
    PHONE_RESTRICTED: HTTP_STATUS_FORBIDDEN,
}
```

## 6. 异常类（对齐官方 accountApiErrorHandler 逻辑）

```python
from dataclasses import dataclass
from typing import Optional

from huawei_constants import (
    SYSTEM_ERROR, INVALID_PARAM, INVALID_CODE, INVALID_CLIENT_SECRET,
    CLIENT_ID_MISMATCH, CODE_EXPIRED, CODE_ALREADY_USED, CODE_CANCELLED,
    CODE_UNAUTHORIZED, NO_PHONE_NUMBER, PHONE_RESTRICTED,
)


@dataclass
class HuaweiAccountError(Exception):
    """
    华为一键登录服务异常

    官方错误处理逻辑（accountApiErrorHandler）：
      1. HTTP 状态码 ≠ 200 → 失败
      2. HTTP 状态码 = 200 → 解析 resultCode
           - resultCode 为 0 或不存在 → 成功
           - resultCode 非 0 → 失败

    使用常量定义错误码，参见常量定义章节
    """
    result_code: Optional[int]   # 华为 resultCode（int），如 60180004
    message: str              # 错误描述
    http_status: Optional[int] = None  # HTTP 状态码

    def __str__(self) -> str:
        rc = self.result_code if self.result_code is not None else "UNKNOWN"
        return f"[{rc}] {self.message}"

    # ── 常用判断 ─────────────────────────────────────────────────────────

    def is_code_expired(self)           -> bool: return self.result_code == CODE_EXPIRED
    def is_code_already_used(self)      -> bool: return self.result_code == CODE_ALREADY_USED
    def is_code_cancelled(self)          -> bool: return self.result_code == CODE_CANCELLED
    def is_code_unauthorized(self)       -> bool: return self.result_code == CODE_UNAUTHORIZED
    def is_no_phone_number(self)         -> bool: return self.result_code == NO_PHONE_NUMBER
    def is_phone_restricted(self)        -> bool: return self.result_code == PHONE_RESTRICTED
    def is_invalid_client_secret(self)    -> bool: return self.result_code == INVALID_CLIENT_SECRET
    def is_client_id_mismatch(self)      -> bool: return self.result_code == CLIENT_ID_MISMATCH
    def is_invalid_code(self)            -> bool: return self.result_code == INVALID_CODE
    def is_invalid_param(self)           -> bool: return self.result_code == INVALID_PARAM
    def is_system_error(self)            -> bool: return self.result_code == SYSTEM_ERROR
```

## 7. 数据结构

```python
from dataclasses import dataclass, field
from typing import Optional

@dataclass
class HuaweiUser:
    """用户信息业务对象"""
    open_id:           Optional[str]
    union_id:          Optional[str]
    phone_number:      Optional[str]     # 含国家码，如 "0086191******08"
    phone_number_valid: int               # 0=需验证，1=可直接使用
    pure_phone_number: Optional[str]     # 不带国家码，如 "191******08"
    phone_country_code: Optional[str]  # 国际冠码+区号，如 "0086"
```

## 7.1 API 接口请求/响应

```python
from dataclasses import dataclass
from typing import Optional

@dataclass
class GetPhoneNumberRequest:
    """一键登录获取手机号请求DTO"""
    code: str  # 客户端SDK返回的Authorization Code（必填）


@dataclass
class GetPhoneNumberResponse:
    """一键登录获取手机号响应DTO"""
    success: bool                   # 是否成功
    result_code: int                # 错误码，0表示成功
    result_desc: str                # 错误描述
    open_id: Optional[str] = None   # 用户OpenID
    union_id: Optional[str] = None  # 用户UnionID
    phone_number: Optional[str] = None       # 华为账号绑定号码（含国家码）
    phone_number_valid: Optional[int] = None # 手机号实时有效性：0=需验证，1=可直接使用
    pure_phone_number: Optional[str] = None  # 不带国家码的手机号
    phone_country_code: Optional[str] = None # 国际冠码+区号
    suggestion: Optional[str] = None          # 业务建议（如展示其他登录方式）

    # ── 静态工厂方法 ─────────────────────────────────────────────────

    @staticmethod
    def success(
        open_id: Optional[str],
        union_id: Optional[str],
        phone_number: Optional[str],
        phone_number_valid: Optional[int],
        pure_phone_number: Optional[str],
        phone_country_code: Optional[str]
    ) -> "GetPhoneNumberResponse":
        """成功响应"""
        from huawei_constants import SUCCESS_CODE
        return GetPhoneNumberResponse(
            success=True,
            result_code=SUCCESS_CODE,
            result_desc="",
            open_id=open_id,
            union_id=union_id,
            phone_number=phone_number,
            phone_number_valid=phone_number_valid,
            pure_phone_number=pure_phone_number,
            phone_country_code=phone_country_code
        )

    @staticmethod
    def fail(result_code: int, result_desc: str) -> "GetPhoneNumberResponse":
        """失败响应"""
        return GetPhoneNumberResponse(
            success=False,
            result_code=result_code,
            result_desc=result_desc
        )

    @staticmethod
    def fail_with_suggestion(result_code: int, result_desc: str, suggestion: str) -> "GetPhoneNumberResponse":
        """失败响应（带建议）"""
        return GetPhoneNumberResponse(
            success=False,
            result_code=result_code,
            result_desc=result_desc,
            suggestion=suggestion
        )

    def to_dict(self) -> dict:
        """转换为字典"""
        data = {
            "success": self.success,
            "resultCode": self.result_code,
            "resultDesc": self.result_desc,
        }
        if self.open_id is not None:
            data["openId"] = self.open_id
        if self.union_id is not None:
            data["unionId"] = self.union_id
        if self.phone_number is not None:
            data["phoneNumber"] = self.phone_number
        if self.phone_number_valid is not None:
            data["phoneNumberValid"] = self.phone_number_valid
        if self.pure_phone_number is not None:
            data["purePhoneNumber"] = self.pure_phone_number
        if self.phone_country_code is not None:
            data["phoneCountryCode"] = self.phone_country_code
        if self.suggestion is not None:
            data["suggestion"] = self.suggestion
        return data
```

## 8. 核心服务类

```python
import os
import yaml
import httpx
from typing import Optional

from huawei_constants import (
    SYSTEM_ERROR, INVALID_PARAM, SUCCESS_CODE, HTTP_OK,
)


def load_config(config_path: str = "config.yaml") -> dict:
    """
    加载配置（优先配置文件，备选环境变量）

    Args:
        config_path: 配置文件路径，默认为 config.yaml

    Returns:
        包含 client_id, client_secret, quick_login_url, server_port 的字典
    """
    # 默认 URL
    default_url = "https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber"

    # 1. 尝试从配置文件读取
    if config_path:
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                config = yaml.safe_load(f)
            oauth = config.get("huawei", {}).get("oauth", {})
            if oauth.get("client-id") and oauth.get("client-secret"):
                return {
                    "client_id": oauth["client-id"],
                    "client_secret": oauth["client-secret"],
                    "quick_login_url": oauth.get("quick-login-url") or default_url,
                    "server_port": config.get("server", {}).get("port", 8080),
                }
        except Exception:
            pass

    # 2. 回退到环境变量
    client_id = os.getenv("HUAWEI_CLIENT_ID", "")
    client_secret = os.getenv("HUAWEI_CLIENT_SECRET", "")

    if not client_id or not client_secret:
        raise ValueError("缺少必要配置")

    return {
        "client_id": client_id,
        "client_secret": client_secret,
        "quick_login_url": default_url,
    }


# ── 全局单例 HTTP 客户端（连接池复用）────────────────────────────────────────
# 性能优化：避免每次请求创建新连接，复用 TCP 通道，减少请求延迟
_http_client: Optional[httpx.Client] = None


def get_http_client() -> httpx.Client:
    """
    获取全局单例 HTTP 客户端（线程安全）

    使用连接池复用技术：
    - 维护长连接（Keep-Alive），避免频繁 TCP 握手
    - 控制并发连接数，避免资源耗尽
    - 自动管理连接生命周期

    Returns:
        httpx.Client: 配置好的 HTTP 客户端实例
    """
    global _http_client
    if _http_client is None:
        # limits 参数用于控制连接池：
        # - max_connections: 最大并发连接数
        # - max_keepalive_connections: 保持的最大空闲连接数
        # - keepalive_expiry: 空闲连接存活时间（秒）
        _http_client = httpx.Client(
            timeout=10.0,
            limits=httpx.Limits(
                max_connections=400,           # 最大并发连接数
                max_keepalive_connections=200, # 保持的最大空闲连接数
                keepalive_expiry=30.0,         # 空闲连接存活时间（秒）
            ),
            http2=True,  # 启用 HTTP/2，提升并发性能
        )
    return _http_client


def close_http_client():
    """
    关闭全局 HTTP 客户端

    应用退出时调用，释放连接池资源
    """
    global _http_client
    if _http_client is not None:
        _http_client.close()
        _http_client = None


class HuaweiAccountClient:
    """
    华为 Account Kit 一键登录客户端

    接口：POST https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber
    Content-Type: application/json
    无需先获取 Access Token，直接调用即可

    错误处理规则（华为官方 CallUtils.accountApiErrorHandler）：
      1. HTTP 状态码 ≠ 200 → 失败
      2. HTTP 状态码 = 200 → 解析 resultCode
           - resultCode 为 0 或不存在 → 成功
           - resultCode 非 0 → 失败

    性能优化：
    - 使用全局单例连接池，避免频繁创建连接
    - 启用 HTTP/2 多路复用，提升并发性能
    - 长连接复用，减少 TCP 握手开销
    """

    QUICK_LOGIN_URL = "https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber"

    def __init__(
        self,
        client_id:     str,
        client_secret: str,
        quick_login_url: str = None,
    ):
        """
        初始化华为账号客户端

        Args:
            client_id: OAuth 2.0 客户端 ID
            client_secret: OAuth 2.0 客户端密钥
            quick_login_url: 可选，自定义接口地址
        """
        self.client_id      = client_id
        self.client_secret  = client_secret
        self.quick_login_url = quick_login_url or self.QUICK_LOGIN_URL
        # 使用全局单例连接池，提升性能
        self.http = get_http_client()

    def close(self):
        """
        关闭客户端（供兼容性使用，实际不关闭全局连接池）
        """
        # 不再关闭全局连接池，由 close_http_client() 统一管理
        pass

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    # ── 公开方法 ─────────────────────────────────────────────────────────

    def quick_login(self, authorization_code: str) -> HuaweiUser:
        """
        一键登录主流程（对齐官方 accountApiErrorHandler 逻辑）

        Args:
            authorization_code: 客户端 SDK 返回的 Authorization Code

        Returns:
            HuaweiUser: 用户信息对象

        Raises:
            HuaweiAccountError: 当 HTTP 请求失败或 resultCode 非 0 时抛出
        """
        try:
            # 发送 POST 请求到华为一键登录接口
            # 使用连接池中的可用连接，避免每次创建新连接
            response = self.http.post(
                self.quick_login_url,
                json={
                    "code":          authorization_code,
                    "clientId":     self.client_id,
                    "clientSecret": self.client_secret,
                },
            )
        except httpx.RequestError as e:
            # 网络请求异常（如 DNS 解析失败、连接超时等）
            raise HuaweiAccountError(
                result_code=SYSTEM_ERROR,
                message=f"网络请求异常: {e}",
                http_status=None
            )

        status = response.status_code
        try:
            body = response.json()
        except Exception:
            body = {}

        # ── 华为官方 accountApiErrorHandler 逻辑 ─────────────────────
        # HTTP 状态码 ≠ 200 → 失败
        if status != HTTP_OK:
            raise HuaweiAccountError(
                result_code=INVALID_PARAM,
                message=f"call failed! http status code: {status}, response data: {response.text}",
                http_status=status
            )

        # HTTP = 200，解析 resultCode
        # resultCode 为 0 或不存在 → 成功
        result_code = body.get("resultCode")
        if result_code is not None and result_code != SUCCESS_CODE:
            raise HuaweiAccountError(
                result_code=int(result_code),
                message=body.get("resultDesc", ""),
                http_status=status
            )

        # 成功返回用户信息
        return HuaweiUser(
            open_id            = body.get("openId"),
            union_id           = body.get("unionId"),
            phone_number       = body.get("phoneNumber"),
            phone_number_valid = body.get("phoneNumberValid", 0),
            pure_phone_number  = body.get("purePhoneNumber"),
            phone_country_code = body.get("phoneCountryCode"),
        )
```

## 9. Flask 接口示例

```python
from flask import Flask, request, jsonify

from huawei_client import (
    HuaweiAccountClient, HuaweiAccountError, load_config, close_http_client
)
from huawei_dto import GetPhoneNumberRequest, GetPhoneNumberResponse
from huawei_constants import (
    SYSTEM_ERROR, INVALID_PARAM, INVALID_CODE, INVALID_CLIENT_SECRET,
    CLIENT_ID_MISMATCH, CODE_EXPIRED, CODE_ALREADY_USED, CODE_CANCELLED,
    CODE_UNAUTHORIZED, NO_PHONE_NUMBER, PHONE_RESTRICTED,
    SUCCESS_CODE, RESULT_CODE_TO_HTTP_STATUS,
)

app = Flask(__name__)


# 接口：POST /huawei/quickLogin/getPhoneNumber
# 默认端口 8080
@app.route("/huawei/quickLogin/getPhoneNumber", methods=["POST"])
def huawei_callback():
    """
    华为一键登录获取手机号接口

    请求体：
        {"code": "客户端SDK返回的Authorization Code"}

    响应：
        成功：{"success": true, "openId": "...", "phoneNumber": "..."}
        失败：{"success": false, "resultCode": 60180004, "resultDesc": "..."}
    """
    # 解析请求
    data = request.get_json(silent=True) or {}
    req = GetPhoneNumberRequest(code=data.get("code", ""))

    # 参数校验：code 为必填项
    if not req.code:
        response = GetPhoneNumberResponse.fail(
            INVALID_PARAM,
            "缺少 Authorization Code"
        )
        return jsonify(response.to_dict()), 400

    # 从配置文件或环境变量加载配置
    cfg = load_config()
    client = HuaweiAccountClient(
        client_id      = cfg["client_id"],
        client_secret  = cfg["client_secret"],
        quick_login_url = cfg.get("quick_login_url"),
    )

    try:
        # 使用 with 语法自动管理客户端（但不关闭全局连接池）
        with client:
            user = client.quick_login(req.code)

        # 返回成功响应
        response = GetPhoneNumberResponse.success(
            open_id=user.open_id,
            union_id=user.union_id,
            phone_number=user.phone_number,
            phone_number_valid=user.phone_number_valid,
            pure_phone_number=user.pure_phone_number,
            phone_country_code=user.phone_country_code
        )
        return jsonify(response.to_dict())

    except HuaweiAccountError as e:
        return handle_huawei_error(e)


def handle_huawei_error(e: HuaweiAccountError):
    """
    统一错误处理（对齐官方 accountApiErrorHandler 逻辑）

    根据不同的 resultCode 返回对应的错误描述和 HTTP 状态码
    """
    # 获取错误码，默认为系统内部错误
    rc = e.result_code if e.result_code else SYSTEM_ERROR

    # 用户无手机号，业务上展示其他登录方式（HTTP 200 返回）
    if e.is_no_phone_number():
        response = GetPhoneNumberResponse.fail_with_suggestion(
            rc,
            "用户无手机号",
            "SHOW_ALTERNATIVE_LOGIN"
        )
        return jsonify(response.to_dict()), 200

    # 根据错误类型映射错误描述
    result_desc = (
        "系统内部错误" if e.is_system_error() else
        "参数不合法" if e.is_invalid_param() else
        "code 参数不正确" if e.is_invalid_code() else
        "clientSecret 参数不正确" if e.is_invalid_client_secret() else
        "code 中的 clientId 和入参不一致" if e.is_client_id_mismatch() else
        "code 过期" if e.is_code_expired() else
        "code 已经被使用过" if e.is_code_already_used() else
        "code 授权被取消" if e.is_code_cancelled() else
        "code 未授权华为账号一键登录权限" if e.is_code_unauthorized() else
        "手机号信息获取受限" if e.is_phone_restricted() else
        e.message
    )

    # 根据错误码映射 HTTP 状态码
    status = RESULT_CODE_TO_HTTP_STATUS.get(e.result_code, 502) if e.result_code else 502

    response = GetPhoneNumberResponse.fail(rc, result_desc)
    return jsonify(response.to_dict()), status


# ── 应用生命周期管理（可选）─────────────────────────────────────────────
# 用于在应用退出时关闭全局连接池，释放资源
@app.teardown_appcontext
def cleanup(exception=None):
    """请求结束后自动清理（可选）"""
    pass  # 全局连接池由 close_http_client() 统一管理


if __name__ == "__main__":
    try:
        # 默认端口 8080
        app.run(host="0.0.0.0", port=8080)
    finally:
        # 应用退出时关闭全局连接池
        close_http_client()
```

## 10. FastAPI 接口示例

```python
from contextlib import asynccontextmanager
from typing import Optional
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import httpx

from huawei_client import (
    HuaweiAccountClient, HuaweiAccountError, HuaweiUser,
    load_config, close_http_client
)
from huawei_dto import GetPhoneNumberResponse
from huawei_constants import (
    SYSTEM_ERROR, INVALID_PARAM, INVALID_CODE, INVALID_CLIENT_SECRET,
    CLIENT_ID_MISMATCH, CODE_EXPIRED, CODE_ALREADY_USED, CODE_CANCELLED,
    CODE_UNAUTHORIZED, NO_PHONE_NUMBER, PHONE_RESTRICTED,
    SUCCESS_CODE, RESULT_CODE_TO_HTTP_STATUS,
)


# ── 全局单例 HTTP 客户端（连接池复用）────────────────────────────────────────
# 性能优化：避免每次请求创建新连接，复用 TCP 通道，减少请求延迟
_http_client: Optional[httpx.AsyncClient] = None


async def get_async_http_client() -> httpx.AsyncClient:
    """
    获取全局异步 HTTP 客户端（支持连接池复用）

    使用连接池复用技术：
    - 维护长连接（Keep-Alive），避免频繁 TCP 握手
    - 启用 HTTP/2 多路复用，提升并发性能
    - 控制并发连接数，避免资源耗尽

    Returns:
        httpx.AsyncClient: 配置好的异步 HTTP 客户端实例
    """
    global _http_client
    if _http_client is None:
        _http_client = httpx.AsyncClient(
            timeout=10.0,
            limits=httpx.Limits(
                max_connections=400,           # 最大并发连接数
                max_keepalive_connections=200, # 保持的最大空闲连接数
                keepalive_expiry=30.0,         # 空闲连接存活时间（秒）
            ),
            http2=True,  # 启用 HTTP/2，提升并发性能
        )
    return _http_client


async def close_async_http_client():
    """
    关闭全局异步 HTTP 客户端

    应用退出时调用，释放连接池资源
    """
    global _http_client
    if _http_client is not None:
        await _http_client.aclose()
        _http_client = None


class CallbackRequest(BaseModel):
    """请求体模型"""
    code: str  # 客户端SDK返回的Authorization Code（必填）


# ── FastAPI 生命周期管理 ────────────────────────────────────────────────────
@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    FastAPI 应用生命周期管理

    - 启动时：预热连接池（可选）
    - 关闭时：关闭全局连接池，释放资源
    """
    # 启动时：预热连接池
    await get_async_http_client()
    yield
    # 关闭时：关闭全局连接池
    await close_async_http_client()


app = FastAPI(lifespan=lifespan)


# 接口：POST /huawei/quickLogin/getPhoneNumber
# 默认端口 8080
@app.post("/huawei/quickLogin/getPhoneNumber", response_model=GetPhoneNumberResponse)
def huawei_callback(req: CallbackRequest):
    """
    华为一键登录获取手机号接口

    请求体：
        {"code": "客户端SDK返回的Authorization Code"}

    响应：
        成功：{"success": true, "openId": "...", "phoneNumber": "..."}
        失败：{"success": false, "resultCode": 60180004, "resultDesc": "..."}

    注意：使用同步函数，内部调用同步的 HuaweiAccountClient.quick_login()
    """
    # 参数校验：code 为必填项
    if not req.code:
        return GetPhoneNumberResponse.fail(
            INVALID_PARAM,
            "缺少 Authorization Code"
        )

    # 从配置文件或环境变量加载配置
    cfg = load_config()
    client = HuaweiAccountClient(
        client_id      = cfg["client_id"],
        client_secret  = cfg["client_secret"],
        quick_login_url = cfg.get("quick_login_url"),
    )

    try:
        # 使用 with 语法自动管理客户端
        with client:
            user = client.quick_login(req.code)

        # 返回成功响应
        return GetPhoneNumberResponse.success(
            open_id=user.open_id,
            union_id=user.union_id,
            phone_number=user.phone_number,
            phone_number_valid=user.phone_number_valid,
            pure_phone_number=user.pure_phone_number,
            phone_country_code=user.phone_country_code
        )

    except HuaweiAccountError as e:
        if e.is_no_phone_number():
            return GetPhoneNumberResponse.fail_with_suggestion(
                e.result_code or SYSTEM_ERROR,
                "用户无手机号",
                "SHOW_ALTERNATIVE_LOGIN"
            )

        # 根据错误码映射 HTTP 状态码
        status = RESULT_CODE_TO_HTTP_STATUS.get(e.result_code, 502) if e.result_code else 502

        # 根据错误类型映射错误描述
        result_desc = (
            "系统内部错误" if e.is_system_error() else
            "参数不合法" if e.is_invalid_param() else
            "code 参数不正确" if e.is_invalid_code() else
            "clientSecret 参数不正确" if e.is_invalid_client_secret() else
            "code 中的 clientId 和入参不一致" if e.is_client_id_mismatch() else
            "code 过期" if e.is_code_expired() else
            "code 已经被使用过" if e.is_code_already_used() else
            "code 授权被取消" if e.is_code_cancelled() else
            "code 未授权华为账号一键登录权限" if e.is_code_unauthorized() else
            "手机号信息获取受限" if e.is_phone_restricted() else
            e.message
        )

        raise HTTPException(
            status_code=status,
            detail=GetPhoneNumberResponse.fail(
                e.result_code or SYSTEM_ERROR,
                result_desc
            ).to_dict()
        )


# 启动服务（默认端口 8080）
# uvicorn main:app --host 0.0.0.0 --port 8080
```

## 11. 全局异常处理器（FastAPI）

```python
from fastapi import Request
from fastapi.responses import JSONResponse

from huawei_client import HuaweiAccountError
from huawei_dto import GetPhoneNumberResponse
from huawei_constants import (
    SYSTEM_ERROR, NO_PHONE_NUMBER, RESULT_CODE_TO_HTTP_STATUS,
)


@app.exception_handler(HuaweiAccountError)
async def huawei_exception_handler(request: Request, exc: HuaweiAccountError):
    """
    全局异常处理器

    统一处理 HuaweiAccountError 异常，根据错误类型返回对应的错误描述和 HTTP 状态码
    """
    # 用户无手机号，业务上展示其他登录方式（HTTP 200 返回）
    if exc.is_no_phone_number():
        response = GetPhoneNumberResponse.fail_with_suggestion(
            exc.result_code or SYSTEM_ERROR,
            "用户无手机号",
            "SHOW_ALTERNATIVE_LOGIN"
        )
        return JSONResponse(
            status_code=200,
            content=response.to_dict(),
        )

    # 根据错误码映射 HTTP 状态码
    status = RESULT_CODE_TO_HTTP_STATUS.get(exc.result_code, 502) if exc.result_code else 502

    # 根据错误类型映射错误描述
    result_desc = (
        "系统内部错误" if exc.is_system_error() else
        "参数不合法" if exc.is_invalid_param() else
        "code 参数不正确" if exc.is_invalid_code() else
        "clientSecret 参数不正确" if exc.is_invalid_client_secret() else
        "code 中的 clientId 和入参不一致" if exc.is_client_id_mismatch() else
        "code 过期" if exc.is_code_expired() else
        "code 已经被使用过" if exc.is_code_already_used() else
        "code 授权被取消" if exc.is_code_cancelled() else
        "code 未授权华为账号一键登录权限" if exc.is_code_unauthorized() else
        "手机号信息获取受限" if exc.is_phone_restricted() else
        exc.message
    )

    response = GetPhoneNumberResponse.fail(
        exc.result_code or SYSTEM_ERROR,
        result_desc
    )
    return JSONResponse(
        status_code=status,
        content=response.to_dict(),
    )


# 启动服务（默认端口 8080）
# uvicorn main:app --host 0.0.0.0 --port 8080
```
