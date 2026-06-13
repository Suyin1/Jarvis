# PHP 参考实现

> 参考 Java 版实现思路，PHP 语言等效代码。来源文件：`references/java.md`
> 依赖：`composer require guzzlehttp/guzzle symfony/yaml`

## 代码说明

本实现遵循 PSR 标准 PHP 编码规范，提供完整的注释说明：

- 每个类、方法都有详细的文档注释（PHPDoc）
- 关键逻辑块包含行内注释说明
- 错误处理清晰明确

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

# Windows
set HUAWEI_CLIENT_ID=your_client_id
set HUAWEI_CLIENT_SECRET=your_client_secret
```

## 3. 安装依赖

```bash
composer require guzzlehttp/guzzle symfony/yaml
```

## 4. 常量定义

```php
<?php
namespace App\Constants;

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
final class HuaweiAccountResultCode
{
    private function __construct() {}

    // ── 错误码常量 ───────────────────────────────────────────────────

    /** 系统内部错误，稍后重试 */
    public const SYSTEM_ERROR = 60010001;

    /** 参数不合法，检查入参 */
    public const INVALID_PARAM = 60010002;

    /** code 参数不正确（伪造或被篡改）*/
    public const INVALID_CODE = 60010012;

    /** clientSecret 参数不正确 */
    public const INVALID_CLIENT_SECRET = 60010013;

    /** code 的 clientId 与入参不一致 */
    public const CLIENT_ID_MISMATCH = 60180003;

    /** code 过期（5分钟有效期）*/
    public const CODE_EXPIRED = 60180004;

    /** code 已被使用过 */
    public const CODE_ALREADY_USED = 60180005;

    /** code 授权被取消 */
    public const CODE_CANCELLED = 60180006;

    /** code 未授权一键登录权限 */
    public const CODE_UNAUTHORIZED = 60180007;

    /** 用户无手机号，展示其他登录方式 */
    public const NO_PHONE_NUMBER = 60180008;

    /** 手机号信息获取受限（用户地域限制）*/
    public const PHONE_RESTRICTED = 60180009;

    // ── HTTP 状态码常量 ─────────────────────────────────────────────

    /** HTTP 请求成功 */
    public const HTTP_OK = 200;

    /** HTTP 客户端错误 - 400 */
    public const HTTP_STATUS_BAD_REQUEST = 400;

    /** HTTP 认证失败 - 401 */
    public const HTTP_STATUS_UNAUTHORIZED = 401;

    /** HTTP 禁止访问 - 403 */
    public const HTTP_STATUS_FORBIDDEN = 403;

    /** HTTP 错误网关 - 502 */
    public const HTTP_STATUS_BAD_GATEWAY = 502;

    // ── 业务常量 ───────────────────────────────────────────────────

    /** 成功响应码 */
    public const SUCCESS_CODE = 0;
}
```

## 4.1 配置加载函数

```php
<?php
namespace App\Utils;

use Symfony\Component\Yaml\Yaml;

/**
 * 配置加载函数（优先配置文件，备选环境变量）
 */
function loadConfig(string $configPath = 'config.yaml'): array
{
    // 默认 URL
    $defaultUrl = 'https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber';

    // 1. 尝试从配置文件读取
    if ($configPath && file_exists($configPath)) {
        try {
            $config = Yaml::parseFile($configPath);
            $huawei = $config['huawei'] ?? [];
            $oauth = $huawei['oauth'] ?? [];
            if (!empty($oauth['client-id']) && !empty($oauth['client-secret'])) {
                return [
                    'client_id' => $oauth['client-id'],
                    'client_secret' => $oauth['client-secret'],
                    'quick_login_url' => $oauth['quick-login-url'] ?: $defaultUrl,
                    'server_port' => ($config['server']['port'] ?? 8080) ?: 8080,
                ];
            }
        } catch (\Exception $e) {
            // 回退到环境变量
        }
    }

    // 2. 回退到环境变量
    $clientId = getenv('HUAWEI_CLIENT_ID') ?: $_ENV['HUAWEI_CLIENT_ID'] ?? '';
    $clientSecret = getenv('HUAWEI_CLIENT_SECRET') ?: $_ENV['HUAWEI_CLIENT_SECRET'] ?? '';

    if (!$clientId || !$clientSecret) {
        throw new \RuntimeException('缺少必要配置');
    }

    return [
        'client_id' => $clientId,
        'client_secret' => $clientSecret,
        'quick_login_url' => $defaultUrl,
    ];
}
```

## 5. 异常类（对齐官方 accountApiErrorHandler 逻辑）

```php
<?php
namespace App\Exceptions;

use App\Constants\HuaweiAccountResultCode;

/**
 * 华为一键登录服务异常
 *
 * 官方错误处理逻辑（accountApiErrorHandler）：
 *   1. HTTP 状态码 ≠ 200 → 失败
 *   2. HTTP 状态码 = 200 → 解析 resultCode
 *        - resultCode 为 0 或不存在 → 成功
 *        - resultCode 非 0 → 失败
 *
 * 使用 HuaweiAccountResultCode 中定义的常量判断错误类型
 */
class HuaweiAccountException extends \Exception
{
    public function __construct(
        public readonly ?int $resultCode, // 华为 resultCode（int），如 60180004
        public readonly ?int $httpStatus,
        string               $message = ''
    ) {
        parent::__construct($message ?: (string)($resultCode ?? '未知错误'));
    }

    // ── 常用判断 ──────────────────────────────────────────────────────

    public function isCodeExpired():           bool { return $this->resultCode === HuaweiAccountResultCode::CODE_EXPIRED; }
    public function isCodeAlreadyUsed():       bool { return $this->resultCode === HuaweiAccountResultCode::CODE_ALREADY_USED; }
    public function isCodeCancelled():         bool { return $this->resultCode === HuaweiAccountResultCode::CODE_CANCELLED; }
    public function isCodeUnauthorized():     bool { return $this->resultCode === HuaweiAccountResultCode::CODE_UNAUTHORIZED; }
    public function isNoPhoneNumber():         bool { return $this->resultCode === HuaweiAccountResultCode::NO_PHONE_NUMBER; }
    public function isPhoneRestricted():      bool { return $this->resultCode === HuaweiAccountResultCode::PHONE_RESTRICTED; }
    public function isInvalidClientSecret():   bool { return $this->resultCode === HuaweiAccountResultCode::INVALID_CLIENT_SECRET; }
    public function isClientIdMismatch():      bool { return $this->resultCode === HuaweiAccountResultCode::CLIENT_ID_MISMATCH; }
    public function isInvalidCode():           bool { return $this->resultCode === HuaweiAccountResultCode::INVALID_CODE; }
    public function isInvalidParam():         bool { return $this->resultCode === HuaweiAccountResultCode::INVALID_PARAM; }
    public function isSystemError():         bool { return $this->resultCode === HuaweiAccountResultCode::SYSTEM_ERROR; }
}
```

## 6. 服务类

```php
<?php
namespace App\Services;

use App\Constants\HuaweiAccountResultCode;
use App\Exceptions\HuaweiAccountException;
use GuzzleHttp\Client;
use GuzzleHttp\Exception\RequestException;

/**
 * 华为 Account Kit 一键登录服务
 *
 * 使用 Guzzle HTTP 客户端，支持连接池复用：
 * - 维护长连接（Keep-Alive），减少 TCP 握手开销
 * - 配置连接超时和读取超时
 * - 自动处理 JSON 序列化/反序列化
 *
 * 错误处理规则（华为官方 CallUtils.accountApiErrorHandler）：
 *   1. HTTP 状态码 ≠ 200 → 失败
 *   2. HTTP 状态码 = 200 → 解析 resultCode
 *        - resultCode 为 0 或不存在 → 成功
 *        - resultCode 非 0 → 失败
 */
class HuaweiAccountService
{
    // 华为一键登录接口（无需先获取 Access Token）
    // 服务端接口：POST /huawei/quickLogin/getPhoneNumber，默认端口 8080
    private const QUICK_LOGIN_URL = 'https://account-api.cloud.huawei.com/oauth2/v6/quickLogin/getPhoneNumber';

    /** @var Client HTTP 客户端（支持连接池） */
    private Client $http;

    /**
     * 构造函数
     *
     * @param string      $clientId      OAuth 2.0 客户端 ID
     * @param string      $clientSecret  OAuth 2.0 客户端密钥
     * @param string|null $quickLoginUrl 可选，自定义接口地址
     */
    public function __construct(
        private string $clientId,
        private string $clientSecret,
        private ?string $quickLoginUrl = null,
    ) {
        // 配置 Guzzle 客户端：超时 10 秒，禁用重定向
        $this->http = new Client([
            'timeout' => 10,
            'http_errors' => false,  // 不自动抛出 HTTP 错误异常
        ]);
    }

    /**
     * 获取接口地址
     *
     * @return string 华为一键登录接口地址
     */
    private function getQuickLoginUrl(): string
    {
        return $this->quickLoginUrl ?? self::QUICK_LOGIN_URL;
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
     * @param string $authorizationCode 客户端 SDK 返回的 Authorization Code
     *
     * @return array 用户信息数组
     * @throws HuaweiAccountException 当请求失败时抛出
     */
    public function quickLogin(string $authorizationCode): array
    {
        try {
            // 发送 POST 请求到华为一键登录接口
            $response = $this->http->post($this->getQuickLoginUrl(), [
                'json' => [
                    'code'         => $authorizationCode,
                    'clientId'    => $this->clientId,
                    'clientSecret'=> $this->clientSecret,
                ],
                'http_errors' => false,  // 不自动抛出 HTTP 错误异常
            ]);

            $status = $response->getStatusCode();
            $body   = $response->getBody()->getContents();
            $data   = json_decode($body, true);

            // ── 华为官方 accountApiErrorHandler 逻辑 ─────────────────
            // HTTP 状态码 ≠ 200 → 失败
            if ($status !== HuaweiAccountResultCode::HTTP_OK) {
                throw new HuaweiAccountException(
                    null, $status,
                    "call failed! http status code: $status, response data: $body"
                );
            }

            // HTTP = 200，解析 resultCode
            // resultCode 为 0 或字段不存在 → 成功
            $resultCode = $data['resultCode'] ?? null;
            if ($resultCode !== null && $resultCode !== HuaweiAccountResultCode::SUCCESS_CODE) {
                throw new HuaweiAccountException(
                    (int)$resultCode,
                    $status,
                    $data['resultDesc'] ?? ''
                );
            }

            // 成功返回用户信息
            return [
                'openId'           => $data['openId'] ?? null,
                'unionId'         => $data['unionId'] ?? null,
                'phoneNumber'      => $data['phoneNumber'] ?? null,
                'phoneNumberValid' => $data['phoneNumberValid'] ?? 0,
                'purePhoneNumber'  => $data['purePhoneNumber'] ?? null,
                'phoneCountryCode' => $data['phoneCountryCode'] ?? null,
            ];

        } catch (RequestException $e) {
            // 网络请求异常（如连接超时、DNS 解析失败等）
            throw new HuaweiAccountException(
                HuaweiAccountResultCode::SYSTEM_ERROR, null,
                '网络请求异常: ' . $e->getMessage()
            );
        }
    }
}
```

## 6.1 API 接口请求/响应

```php
<?php
namespace App\DTO;

/**
 * 一键登录获取手机号请求DTO
 */
class GetPhoneNumberRequest
{
    public function __construct(
        public readonly string $code  // 客户端SDK返回的Authorization Code（必填）
    ) {}

    public static function fromArray(array $data): self
    {
        return new self(
            code: $data['code'] ?? ''
        );
    }
}

/**
 * 一键登录获取手机号响应DTO
 */
class GetPhoneNumberResponse
{
    public function __construct(
        public readonly bool   $success,           // 是否成功
        public readonly int    $resultCode,        // 错误码，0表示成功
        public readonly string $resultDesc,        // 错误描述
        public readonly ?string $openId = null,    // 用户OpenID
        public readonly ?string $unionId = null,   // 用户UnionID
        public readonly ?string $phoneNumber = null,      // 华为账号绑定号码（含国家码）
        public readonly ?int    $phoneNumberValid = null, // 手机号实时有效性：0=需验证，1=可直接使用
        public readonly ?string $purePhoneNumber = null,  // 不带国家码的手机号
        public readonly ?string $phoneCountryCode = null, // 国际冠码+区号
        public ?string $suggestion = null,          // 业务建议（如展示其他登录方式）
    ) {}

    // ── 静态工厂方法 ─────────────────────────────────────────────────

    /** 成功响应 */
    public static function success(
        ?string $openId,
        ?string $unionId,
        ?string $phoneNumber,
        ?int $phoneNumberValid,
        ?string $purePhoneNumber,
        ?string $phoneCountryCode
    ): self {
        return new self(
            success:          true,
            resultCode:       \App\Constants\HuaweiAccountResultCode::SUCCESS_CODE,
            resultDesc:       '',
            openId:           $openId,
            unionId:          $unionId,
            phoneNumber:      $phoneNumber,
            phoneNumberValid: $phoneNumberValid,
            purePhoneNumber:  $purePhoneNumber,
            phoneCountryCode: $phoneCountryCode
        );
    }

    /** 失败响应 */
    public static function fail(int $resultCode, string $resultDesc): self
    {
        return new self(
            success:    false,
            resultCode: $resultCode,
            resultDesc: $resultDesc
        );
    }

    /** 失败响应（带建议）*/
    public static function failWithSuggestion(int $resultCode, string $resultDesc, string $suggestion): self
    {
        return new self(
            success:    false,
            resultCode: $resultCode,
            resultDesc: $resultDesc,
            suggestion: $suggestion
        );
    }

    /** 转换为数组 */
    public function toArray(): array
    {
        $data = [
            'success'    => $this->success,
            'resultCode' => $this->resultCode,
            'resultDesc' => $this->resultDesc,
        ];

        if ($this->openId !== null)           $data['openId'] = $this->openId;
        if ($this->unionId !== null)          $data['unionId'] = $this->unionId;
        if ($this->phoneNumber !== null)      $data['phoneNumber'] = $this->phoneNumber;
        if ($this->phoneNumberValid !== null) $data['phoneNumberValid'] = $this->phoneNumberValid;
        if ($this->purePhoneNumber !== null)  $data['purePhoneNumber'] = $this->purePhoneNumber;
        if ($this->phoneCountryCode !== null) $data['phoneCountryCode'] = $this->phoneCountryCode;
        if ($this->suggestion !== null)       $data['suggestion'] = $this->suggestion;

        return $data;
    }
}
```

## 7. Laravel Controller 示例

```php
<?php
// routes/api.php
use App\Constants\HuaweiAccountResultCode;
use App\DTO\GetPhoneNumberRequest;
use App\DTO\GetPhoneNumberResponse;
use App\Services\HuaweiAccountService;
use App\Exceptions\HuaweiAccountException;

/**
 * 统一错误处理函数
 *
 * @param HuaweiAccountException $e 华为账号异常
 * @return \Illuminate\Http\JsonResponse
 */
function handleHuaweiException(HuaweiAccountException $e): \Illuminate\Http\JsonResponse
{
    $rc = $e->resultCode ?? HuaweiAccountResultCode::SYSTEM_ERROR;

    // 用户无手机号，业务上展示其他登录方式
    if ($e->isNoPhoneNumber()) {
        $response = GetPhoneNumberResponse::failWithSuggestion(
            $rc,
            '用户无手机号',
            'SHOW_ALTERNATIVE_LOGIN'
        );
        return response()->json($response->toArray(), 200);
    }

    $resultDesc = match (true) {
        $e->isSystemError()         => '系统内部错误',
        $e->isInvalidParam()       => '参数不合法',
        $e->isInvalidCode()         => 'code 参数不正确',
        $e->isInvalidClientSecret() => 'clientSecret 参数不正确',
        $e->isClientIdMismatch()   => 'code 中的 clientId 和入参不一致',
        $e->isCodeExpired()       => 'code 过期',
        $e->isCodeAlreadyUsed()   => 'code 已经被使用过',
        $e->isCodeCancelled()     => 'code 授权被取消',
        $e->isCodeUnauthorized() => 'code 未授权华为账号一键登录权限',
        $e->isPhoneRestricted()   => '手机号信息获取受限',
        default                     => $e->getMessage(),
    };

    $status = match (true) {
        $e->isSystemError()         => HuaweiAccountResultCode::HTTP_STATUS_BAD_GATEWAY,
        $e->isInvalidParam(), $e->isInvalidCode(),
        $e->isCodeExpired(), $e->isCodeAlreadyUsed(),
        $e->isCodeCancelled(), $e->isCodeUnauthorized(),
        $e->isClientIdMismatch()   => HuaweiAccountResultCode::HTTP_STATUS_BAD_REQUEST,
        $e->isInvalidClientSecret()=> HuaweiAccountResultCode::HTTP_STATUS_UNAUTHORIZED,
        $e->isPhoneRestricted()    => HuaweiAccountResultCode::HTTP_STATUS_FORBIDDEN,
        default                     => HuaweiAccountResultCode::HTTP_STATUS_BAD_GATEWAY,
    };

    $response = GetPhoneNumberResponse::fail($rc, $resultDesc);
    return response()->json($response->toArray(), $status);
}

// 接口：POST /huawei/quickLogin/getPhoneNumber
// 默认端口 8080
Route::post('/huawei/quickLogin/getPhoneNumber', function () {
    // 解析请求
    $request = GetPhoneNumberRequest::fromArray(request()->all());

    if (empty($request->code)) {
        $response = GetPhoneNumberResponse::fail(
            HuaweiAccountResultCode::INVALID_PARAM,
            '缺少 Authorization Code'
        );
        return response()->json($response->toArray(), 400);
    }

    try {
        $service = new HuaweiAccountService(
            clientId:     config('services.huawei.client_id'),
            clientSecret: config('services.huawei.client_secret'),
        );

        $userInfo = $service->quickLogin($request->code);

        // 返回成功响应
        $response = GetPhoneNumberResponse::success(
            openId:           $userInfo['openId'] ?? null,
            unionId:          $userInfo['unionId'] ?? null,
            phoneNumber:      $userInfo['phoneNumber'] ?? null,
            phoneNumberValid: $userInfo['phoneNumberValid'] ?? null,
            purePhoneNumber:  $userInfo['purePhoneNumber'] ?? null,
            phoneCountryCode: $userInfo['phoneCountryCode'] ?? null
        );

        return response()->json($response->toArray());

    } catch (HuaweiAccountException $e) {
        return handleHuaweiException($e);
    }
});
```

## 8. 独立 PHP（无框架）示例

```php
<?php
require_once __DIR__ . '/constants/HuaweiAccountResultCode.php';
require_once __DIR__ . '/Exceptions/HuaweiAccountException.php';
require_once __DIR__ . '/DTO/GetPhoneNumberRequest.php';
require_once __DIR__ . '/DTO/GetPhoneNumberResponse.php';
require_once __DIR__ . '/Services/HuaweiAccountService.php';

use App\Constants\HuaweiAccountResultCode;
use App\DTO\GetPhoneNumberRequest;
use App\DTO\GetPhoneNumberResponse;

// 接口：POST /huawei/quickLogin/getPhoneNumber
// 默认端口 8080

header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    $response = GetPhoneNumberResponse::fail(
        HuaweiAccountResultCode::INVALID_PARAM,
        '只支持 POST'
    );
    http_response_code(405);
    echo json_encode($response->toArray());
    exit;
}

$input = json_decode(file_get_contents('php://input'), true);
// 解析请求
$request = GetPhoneNumberRequest::fromArray($input ?? []);

if (empty($request->code)) {
    $response = GetPhoneNumberResponse::fail(
        HuaweiAccountResultCode::INVALID_PARAM,
        '缺少 Authorization Code'
    );
    http_response_code(400);
    echo json_encode($response->toArray());
    exit;
}

try {
    // 从配置文件或环境变量加载配置
    $config = loadConfig('config.yaml');
    $service = new HuaweiAccountService(
        clientId:        $config['client_id'],
        clientSecret:    $config['client_secret'],
        quickLoginUrl:   $config['quick_login_url'] ?? null,
    );

    $userInfo = $service->quickLogin($request->code);

    // 返回成功响应
    $response = GetPhoneNumberResponse::success(
        openId:           $userInfo['openId'] ?? null,
        unionId:          $userInfo['unionId'] ?? null,
        phoneNumber:      $userInfo['phoneNumber'] ?? null,
        phoneNumberValid: $userInfo['phoneNumberValid'] ?? null,
        purePhoneNumber:  $userInfo['purePhoneNumber'] ?? null,
        phoneCountryCode: $userInfo['phoneCountryCode'] ?? null
    );
    echo json_encode($response->toArray());

} catch (HuaweiAccountException $e) {
    $rc = $e->resultCode ?? HuaweiAccountResultCode::SYSTEM_ERROR;

    if ($e->isNoPhoneNumber()) {
        $response = GetPhoneNumberResponse::failWithSuggestion(
            $rc,
            '用户无手机号',
            'SHOW_ALTERNATIVE_LOGIN'
        );
        echo json_encode($response->toArray());
        exit;
    }

    $resultDesc = match (true) {
        $e->isSystemError()         => '系统内部错误',
        $e->isInvalidParam()       => '参数不合法',
        $e->isInvalidCode()         => 'code 参数不正确',
        $e->isInvalidClientSecret() => 'clientSecret 参数不正确',
        $e->isClientIdMismatch()   => 'code 中的 clientId 和入参不一致',
        $e->isCodeExpired()       => 'code 过期',
        $e->isCodeAlreadyUsed()   => 'code 已经被使用过',
        $e->isCodeCancelled()     => 'code 授权被取消',
        $e->isCodeUnauthorized() => 'code 未授权华为账号一键登录权限',
        $e->isPhoneRestricted()   => '手机号信息获取受限',
        default                     => $e->getMessage(),
    };

    $status = match (true) {
        $e->isSystemError()         => HuaweiAccountResultCode::HTTP_STATUS_BAD_GATEWAY,
        $e->isInvalidParam(), $e->isInvalidCode(),
        $e->isCodeExpired(), $e->isCodeAlreadyUsed(),
        $e->isCodeCancelled(), $e->isCodeUnauthorized(),
        $e->isClientIdMismatch()   => HuaweiAccountResultCode::HTTP_STATUS_BAD_REQUEST,
        $e->isInvalidClientSecret()=> HuaweiAccountResultCode::HTTP_STATUS_UNAUTHORIZED,
        $e->isPhoneRestricted()    => HuaweiAccountResultCode::HTTP_STATUS_FORBIDDEN,
        default                     => HuaweiAccountResultCode::HTTP_STATUS_BAD_GATEWAY,
    };

    $response = GetPhoneNumberResponse::fail($rc, $resultDesc);
    http_response_code($status);
    echo json_encode($response->toArray());
}
```
