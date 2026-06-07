---
audience: ai-agent
priority: high
purpose: Vale 集成细节，包括离线证明、规则系统和可持续性
category: reference
last-updated: 2026-06-03
---

# Vale 检查集成

> 资料解决方案系统中如何使用 Vale 进行文档质量检查

---

## 什么是 Vale

[Vale](https://vale.sh) 是一个独立的开源文档检查工具。它根据可配置的风格规则检查 Markdown 文件的语法、风格、术语和可读性。

在资料解决方案系统中，Vale 作为**风格和格式检查后端**。它是可选的——系统在未安装 Vale 时仍能优雅降级运行。

## Vale 的集成方式

### 架构

```
+-------------------+     Vale JSON 输出      +-------------------+
|  ValeAdapter      | ------------------------> |  CheckReport      |
|  (Python 封装)     |     解析告警            |  (统一格式)       |
+-------------------+                           +-------------------+
          |                                                ^
          | vale CLI --output json                         |
          v                                                |
+-------------------+                                    |
|  vale 二进制       |     可选：通过 npm 安装              |
|  (vale.exe)        |     或从 vale.sh 下载               |
+-------------------+
```

集成位于 `engine/rule_engine/vale_adapter.py`：

### ValeAdapter 类

```python
class ValeAdapter:
    def __init__(self, config: ValeConfig):
        # config.vale_bin: vale 可执行文件路径（默认："vale"）
        # config.config_path: .vale.ini 路径（可选）

    def check(self, target: str) -> ValeResult:
        # 1. 验证目标路径是否存在
        # 2. 构建 vale CLI 命令
        # 3. 执行：vale --output JSON <target>
        # 4. 将 JSON 输出解析为 ValeResult
        # 5. 返回结构化结果
```

### 优雅降级

当 Vale 未安装时：

1. `ValeAdapter.check()` 检测到 vale CLI 未找到
2. 返回 `ValeResult`，其中 `exit_code = -1` 并带错误信息
3. `tools/check.py` 将其转换为 `ReportItem`，严重级别为 `warning`
4. 系统继续执行结构和格式检查
5. 不会崩溃，不会中断

## Vale 配置

### 默认配置

默认 Vale 配置（`knowledge/rules/vale/.vale.ini`）：

```ini
StylesPath = styles
MinAlertLevel = suggestion

[*.md]
BasedOnStyles = DocsStyle, Custom

[DocsStyle]
HeadingHierarchy = YES
SentenceLength = YES

[Custom]
Terminology = YES
```

### 生成的配置

运行 `build-kb` 时，会生成客户特定的 Vale 配置：

```ini
; Vale 配置 - 客户名称
; 由资料解决方案知识库构建器生成

StylesPath = styles
MinAlertLevel = suggestion

[*.md]
BasedOnStyles = DocsStyle, Custom

[DocsStyle]
HeadingHierarchy = YES
SentenceLength = YES

[Custom]
Terminology = YES
```

## 系统中的 Vale 规则

### 内置规则（`knowledge/rules/vale/styles/`）

#### DocsStyle/HeadingHierarchy.yml

```yaml
extends: existence
message: "标题级别应使用 '#' 标记"
level: warning
scope: heading
nonword: true
tokens:
  - '#+ '
```

#### Custom/Terminology.yml

```yaml
extends: conditional
message: "术语 '%s' 可能不符合规范"
level: suggestion
scope: text
ignorecase: true
first: '\\b(API|SDK|UI|JSON|YAML|CLI|REST|HTTP|SSH|MCP)\\b'
second: ''
action:
  name: replace
  params:
    - API
    - API
```

### 添加自定义规则

将自定义 Vale 规则放在 `knowledge/rules/vale/styles/` 中：

```
knowledge/rules/vale/styles/
  |-- DocsStyle/
  |   |-- HeadingHierarchy.yml
  |   +-- SentenceLength.yml
  |-- Custom/
  |   |-- Terminology.yml
  |   +-- BrandName.yml
  +-- YourOrg/
      +-- YourRule.yml
```

然后在 `.vale.ini` 中引用：

```ini
[*.md]
BasedOnStyles = DocsStyle, Custom, YourOrg
```

## 检查类型与 Vale

`check` 命令根据 `--check-type` 参数集成 Vale：

| --check-type | 使用 Vale？ | 说明 |
|-------------|------------|------|
| `structure` | 否 | 标题层级、必需章节 |
| `format` | 是 | Vale 风格检查 + 内置格式检查（段落长度、代码块） |
| `style` | 是 | 仅 Vale 风格检查 |
| `all` | 是 | 全部：结构 + 格式 + 风格 |

## 运行 Vale 检查

### 通过 CLI

```bash
# 包含 Vale 的完整检查
doc-solution check --target ./docs/ --check-type all

# 仅风格检查（使用 Vale）
doc-solution check --target ./docs/ --check-type style

# 格式检查（使用 Vale + 内置）
doc-solution check --target ./docs/ --check-type format

# 自定义 Vale 配置
doc-solution check --target ./docs/ --config ./custom/.vale.ini
```

### 通过 MCP

`quality_check` MCP 工具支持相同的参数：

```json
{
  "name": "quality_check",
  "arguments": {
    "target": "./docs/",
    "check_type": "all",
    "vale_bin": "vale",
    "config_path": "./custom/.vale.ini"
  }
}
```

### Vale JSON 输出

Vale 输出 JSON，由适配器解析：

```json
{
  "test.md": {
    "alerts": [
      {
        "Line": 1,
        "Column": 1,
        "Severity": "warning",
        "Message": "使用 'API' 而不是 'api'。",
        "Check": "Custom.Terminology",
        "Match": "api"
      }
    ]
  }
}
```

由 `ValeAdapter` 转换为系统的 `CheckReport` 格式。

## 安装/使用 Vale

### 方式 1：内置二进制（离线，推荐）

本项目包含一个预编译的 Vale 二进制文件，用于**内网环境的离线使用**：

```
knowledge/vale.exe   （Windows，约 38 MB）
```

当 Vale 不在 PATH 中时，系统自动检测此内置二进制文件。你也可以明确指定：

```bash
# 自动检测（Vale 不在 PATH 时使用内置二进制）
doc-solution check --target ./docs/

# 显式指定内置二进制路径
doc-solution check --target ./docs/ --vale-bin knowledge/vale.exe
```

> **Linux/Mac 用户**：将 `vale.exe` 替换为来自 https://github.com/errata-ai/vale/releases 的相应二进制文件

### 方式 2：通过 npm 安装（需要网络）

```bash
npm install -g @errata-ai/vale
```

### 方式 3：手动下载（用于内网传输）

如果在另一台机器上有网络访问权限：

1. 从 https://github.com/errata-ai/vale/releases 下载
2. 将二进制文件复制到目标机器
3. 放入 `knowledge/bin/` 或 PATH 中的任意目录
4. 验证：

```bash
vale --version
```

### 本项目中 Vale 二进制的位置

| 项目 | 路径 |
|------|------|
| 内置二进制（Windows） | `knowledge/vale.exe` |
| 默认 Vale 配置 | `knowledge/rules/vale/.vale.ini` |
| Vale 风格规则 | `knowledge/rules/vale/styles/` |
| 源文件（npm 全局） | `%APPDATA%/npm/vale.exe` |

## 安全：零网络活动

Vale 是一个**完全离线**的工具。它不执行任何网络操作：

- 无 DNS 查询
- 无 HTTP/HTTPS 请求
- 无遥测或分析
- 无更新检查
- 无许可证验证
- 无外部服务依赖

所有操作都是本地的：
1. 从本地文件系统读取 `.vale.ini`
2. 从本地 `styles/` 目录加载风格规则
3. 从本地路径解析目标文件
4. 应用规则并将结果输出到 stdout

内置二进制（`knowledge/vale.exe`）没有嵌入网络代码。即使是内置风格（如 "Vale.Spelling" 和 "Microsoft.*"）也直接编译进二进制文件，不会在运行时下载。

详见 `docs/customer/SECURITY.md` 的完整安全和隐私细节。

## Vale 二进制的可持续性

### 内置二进制可以无限期使用吗？

**可以，对于基于 YAML 的自定义场景。** Vale 的规则系统是配置驱动的，而非代码驱动：

| 自定义类型 | 需要重新编译？ | 方式 |
|-----------|--------------|------|
| 添加新术语规则 | **不需要** | 编辑 `knowledge/rules/vale/styles/Custom/` 中的 YAML |
| 添加品牌名称检查 | **不需要** | 创建新的 `.yml` 规则文件 |
| 更改告警级别 | **不需要** | 编辑 `.vale.ini` |
| 添加拼写词典 | **不需要** | 添加词汇文件 |
| 移除默认检查 | **不需要** | 在 `.vale.ini` 中配置 |
| 基于脚本的规则（Go 插件）| **需要** | 需要自定义 Vale 构建 |

### 支持的规则类型（全部为 YAML，无需重新编译）

| 类型 | 用途 | 示例 |
|------|------|------|
| `existence` | 检查是否包含某些术语 | "不要使用'请'" |
| `substitution` | 替换不正确的术语 | "使用'API'而不是'api'" |
| `occurrence` | 限制术语频率 | "每页最多 3 个脚注" |
| `repetition` | 检测重复词 | "the the" |
| `consistency` | 强制一致使用 | "统一使用'e-mail'或'email'" |
| `conditional` | 条件检查 | "如果使用'click'，也要提及'tap'" |
| `capitalization` | 检查大小写模式 | "产品名称应大写" |
| `spelling` | 自定义拼写检查 | 添加领域特定术语 |
| `metric` | 可读性指标 | "Flesch 阅读分数 > 60" |

### 何时需要新的二进制文件？

1. **Go 插件脚本**——如果需要包含自定义 Go 代码的 `script` 类型规则
2. **新 Vale 版本**——如果更新的 Vale 版本提供了所需功能
3. **不同平台**——Linux/Mac 用户需要特定平台的二进制文件

对于绝大多数文档检查需求（术语、标题层级、格式、风格），内置二进制 + YAML 规则**完全足够**。

## 故障排查

### Vale 未找到

如果未安装 Vale，系统会显示：

```
[?] [needs_review] Vale 不可用。安装方法：npm install -g @errata-ai/vale
```

检查将继续执行结构和格式检查。

### Vale 配置未找到

如果 `.vale.ini` 路径不正确：

```
[?] [needs_review] 未找到 Vale 配置：./path/to/.vale.ini
```

使用 `--config` 指定正确路径。

### Vale JSON 解析错误

如果 Vale 输出意外格式，适配器会返回错误信息并继续运行。
