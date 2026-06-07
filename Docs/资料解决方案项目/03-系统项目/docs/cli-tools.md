---
audience: ai-agent
priority: high
purpose: AI Agent 使用的完整 CLI 命令参考
category: reference
last-updated: 2026-06-03
---

# CLI 工具参考

> 资料解决方案 CLI 工具的完整参考

---

## 安装

```bash
# 从项目根目录
pip install -e .

# 验证
doc-solution --help
```

或无需安装直接运行：

```bash
python -m tools.cli --help
```

## 全局选项

```
--help      显示帮助信息
--version   显示版本（0.1.0）
```

## 命令：check

对文档/代码运行质量检查。

### 用法

```bash
doc-solution check --target <路径> [选项]
```

### 选项

| 选项 | 别名 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--target` | `-t` | 是 | - | 目标文件或目录 |
| `--check-type` | `-c` | 否 | `all` | 检查类型：`all`、`structure`、`format`、`style` |
| `--output` | `-o` | 否 | `text` | 输出格式：`text`、`json` |
| `--vale-bin` | - | 否 | `vale` | Vale 可执行文件路径 |
| `--config` | - | 否 | - | Vale 配置文件路径 |
| `--save-report` | - | 否 | - | 将 JSON 报告保存到文件 |

### 检查类型

| 类型 | 检查内容 | 依赖 |
|------|---------|------|
| `structure` | 标题层级、必需章节 | MDParser（内置） |
| `format` | 段落长度、代码块语言、Vale 风格 | Vale（可选） |
| `style` | 术语、品牌名称、Vale 风格规则 | Vale（可选） |
| `all` | 以上全部 | 两者 |

### 示例

```bash
# 基本检查目录
doc-solution check --target ./knowledge/templates/

# 检查单个文件
doc-solution check --target ./docs/api-reference.md

# 仅结构检查
doc-solution check --target ./docs/ --check-type structure

# JSON 输出（用于程序化使用）
doc-solution check --target ./docs/ --output json

# JSON 输出保存到文件
doc-solution check --target ./docs/ --output json --save-report report.json

# 使用自定义 Vale 配置
doc-solution check --target ./docs/ --config ./knowledge/rules/vale/.vale.ini
```

### 退出码

| 编码 | 含义 |
|------|------|
| 0 | 所有检查通过（无错误） |
| 1 | 发现错误或目标未找到 |

## 命令：generate

从 Jinja2 模板生成文档内容。

### 用法

```bash
doc-solution generate --template <名称> --params <json> [选项]
```

### 选项

| 选项 | 别名 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--template` | `-t` | 是 | - | 模板名称或文件路径 |
| `--params` | `-p` | 否 | `{}` | 模板参数（JSON） |
| `--template-dir` | - | 否 | `knowledge/templates` | 模板搜索目录 |
| `--output` | `-o` | 否 | stdout | 输出文件路径 |
| `--auto-check` | - | 否 | true | 自动运行质量检查 |

### 模板解析

`--template` 值按以下顺序解析：

1. 如果是文件路径，直接使用该文件
2. 如果是 `--template-dir` 中的目录名，使用目录内的 `.j2` 文件
3. 否则，追加 `.md.j2` 并在 `--template-dir` 中搜索

### 示例

```bash
# 从命名模板生成（输出到 stdout）
doc-solution generate --template api-ref --params '{"api_name": "startAbility"}'

# 生成并保存到文件
doc-solution generate --template api-ref --params '{...}' --output ./output/api.md

# 使用自定义模板目录
doc-solution generate --template custom-template --template-dir ./my-templates/

# 使用特定模板文件
doc-solution generate --template ./path/to/template.md.j2 --params '{...}'

# 禁用自动检查
doc-solution generate --template api-ref --params '{...}' --no-auto-check
```

### 内置模板

| 模板 | 说明 | 关键参数 |
|------|------|----------|
| `api-ref` | API 参考文档 | `api_name`、`declaration`、`parameters[]`、`return_type`、`error_codes[]` |
| `dev-guide` | 开发指南 | `title`、`overview`、`prerequisites[]`、`steps[]` |

## 命令：build-kb

从客户源材料构建知识库。

### 用法

```bash
doc-solution build-kb --input <目录> --name <名称> [选项]
```

### 选项

| 选项 | 别名 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--input` | `-i` | 是 | - | 源材料目录 |
| `--name` | `-n` | 是 | - | 客户名称 |
| `--output` | `-o` | 否 | `knowledge` | 输出目录 |
| `--force` | - | 否 | false | 覆盖现有知识库 |

### 示例

```bash
# 基本构建
doc-solution build-kb --input ./customer-inputs/ --name "Huawei-HarmonyOS"

# 指定输出目录
doc-solution build-kb --input ./customer-inputs/ --name "华为" --output ./kb-huawei/

# 覆盖现有知识库
doc-solution build-kb --input ./customer-inputs/ --name "华为" --force
```

### 输出

命令创建以下结构：

```
<output>/
  |-- config.yaml               # 知识库配置
  |-- rules/vale/.vale.ini      # Vale 配置
  |-- rules/vale/styles/        # Vale 风格规则
  |-- rules/custom/             # 自定义规则模板
  |-- templates/                # 注册的模板
  |-- glossary/terms.yaml       # 空术语文件
  |-- checklist/                # 空检查清单文件
  +-- meta/style-profile.yaml   # 风格分析
```

## 命令：test-rule

验证 Vale 规则的正向和负向测试。

### 用法

```bash
doc-solution test-rule --rule <规则文件.yml> [选项]
```

### 选项

| 选项 | 别名 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--rule` | `-r` | 是 | - | Vale 规则 YAML 文件路径 |
| `--should-fail` | - | 否 | - | 应触发规则的 .md 文件 |
| `--should-pass` | - | 否 | - | 不应触发规则的 .md 文件 |
| `--output` | `-o` | 否 | `text` | 输出格式：`text`、`json` |

### 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 通过——所有测试通过 |
| 1 | 失败——正向或负向测试未通过 |
| 1 | 语法错误——规则文件存在语法错误 |

### 示例

```bash
# 基本语法验证（仅规则文件）
doc-solution test-rule --rule rules/vale/styles/Custom/TermSubstitution.yml

# 完整测试（正向 + 负向）
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --should-pass ./tests/should-pass.md

# JSON 输出
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --output json
```

### 输出结构

```bash
doc-solution test-rule --rule Custom.ProductName --output text
```

## 命令：doc-solution-mcp

以 stdio 模式运行 MCP 服务端（用于 AI Agent 集成）。

```bash
doc-solution-mcp
# 或
python -m mcp.server
```

详见 `docs/mcp-server.md`。
