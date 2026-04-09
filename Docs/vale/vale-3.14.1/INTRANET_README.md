# Vale Intranet Edition (内网安全版)

本版本基于 [Vale v3.14.1](https://github.com/errata-ai/vale)，专为**内网环境**设计。

## 安全特性

### 已禁用的网络功能

| 功能 | 原命令 | 状态 |
|------|--------|------|
| 同步远程样式包 | `vale sync` | 已禁用 |
| 安装远程样式 | `vale install` | 已禁用 |
| 安装浏览器扩展 | `vale host-install` | 已禁用 |
| 卸载浏览器扩展 | `vale host-uninstall` | 已禁用 |
| 外部NLP API | `NLPEndpoint` 配置 | 已禁用 |

### 安全保证

- 所有网络请求已移除
- 配置文件中的 `NLPEndpoint` 配置项将被忽略
- `.vale.ini` 中的 `Packages` 配置需要手动下载安装
- 不进行任何外部HTTP请求

## 使用方法

### 基本检查

```bash
vale-intranet.exe file.md
vale-intranet.exe --config=.vale.ini docs/
```

### 手动安装样式包

1. 从可信来源下载样式包（如GitHub releases）
2. 解压到 `StylesPath` 目录
3. 在 `.vale.ini` 中配置样式名称

### 配置文件示例

```ini
StylesPath = .github/styles
MinAlertLevel = suggestion
Vocab = MyProject

[*.md]
BasedOnStyles = Vale, Microsoft
```

## 编译

```bash
go build -ldflags="-s -w" -o vale-intranet.exe ./cmd/vale
```

## 注意事项

- 本版本不执行任何网络请求
- 样式包需要手动下载和更新
- 不支持外部NLP服务（如需要，请部署本地NLP服务）
- 所有功能仅限本地文件操作

## 许可证

继承自 [Vale](https://github.com/errata-ai/vale) 的 MIT 许可证。
