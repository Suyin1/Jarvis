# 资料解决方案系统 — AI Agent 使用指南

> 如果你是 AI Agent，请先阅读本文档了解如何调用本系统的能力。

---

## 一、系统概述

本系统是**资料开发全链路解决方案**的 CLI 工具集。当前版本（v0.1）提供三个核心命令：

| 命令 | 功能 | 当前状态 |
|------|------|----------|
| `doc-solution check` | 文档质量检查 | ✅ 可用 |
| `doc-solution generate` | 基于模板生成内容 | ✅ 可用 |
| `doc-solution build-kb` | 从客户输入构建知识库 | ✅ 可用 |

## 二、工作流程

### 2.1 完整接入流程

```
新客户接入 →
    1. build-kb  ← 分析客户源材料，构建知识库
    2. check     ← 验证知识库规则是否准确
    3. generate  ← 基于知识库模板生成内容
    4. check     ← 验证生成内容质量
```

### 2.2 日常使用流程

```
日常开发 →
    1. generate  ← 根据需求生成文档内容
    2. check     ← 自动质量检查
    3. 人工审核  ← 确认输出

日常维护 →
    1. check     ← 扫描已有文档质量
    2. 分析报告  ← 定位问题
    3. 修复     ← 根据问题修改

问题解决 →
    1. 分析问题单
    2. check --check-type all ← 全量检查定位问题
    3. 修复验证
```

## 三、命令详解

### 3.1 `doc-solution check` — 质量检查

```bash
# 基本用法
doc-solution check --target ./docs/ --check-type all

# 参数说明
--target, -t     # 必填: 待检查的文件或目录
--check-type, -c # 可选: all/structure/format/style/code/consistency (默认: all)
--output, -o     # 可选: text/json (默认: text)
--vale-bin       # 可选: Vale 可执行文件路径 (默认: vale)
--config         # 可选: Vale 配置文件路径
--save-report    # 可选: 保存报告到文件
```

**检查类型说明：**

| 类型 | 检查内容 | 依赖 |
|------|----------|------|
| `structure` | 标题层级、章节结构、代码块标注 | 无 |
| `format` | 段落长度、格式规范 (含 Vale) | Vale (可选) |
| `style` | 术语一致、风格规范 | Vale (可选) |
| `all` | 以上全部 | — |

**输出示例（JSON）：**

```json
{
  "metadata": { "check_id": "check-...", "check_type": "structure", "target": "./docs/" },
  "summary": { "total": 3, "passed": 0, "failed": 1, "warnings": 2, "score": 70.0 },
  "details": [
    {
      "rule_id": "heading-level",
      "severity": "error",
      "message": "缺少一级标题 (H1)",
      "file": "docs/test.md",
      "line": 1
    }
  ],
  "trace": [
    { "step": "结构检查", "tool": "md_parser", "status": "completed" }
  ]
}
```

### 3.2 `doc-solution generate` — 内容生成

```bash
# 基本用法
doc-solution generate --template api-ref --params '{"api_name": "startAbility"}'

# 参数说明
--template, -t  # 必填: 模板名称 (api-ref/dev-guide) 或模板文件路径
--params, -p    # 必填: JSON 格式的模板参数
--template-dir  # 可选: 模板目录 (默认: knowledge/templates)
--output, -o    # 可选: 输出文件路径
--auto-check    # 可选: 生成后自动检查 (默认开启)
```

**可用模板：**

| 模板名 | 说明 | 必需参数 |
|--------|------|----------|
| `api-ref` | API 参考文档 | `api_name`, `declaration` |
| `dev-guide` | 开发指南 | `title`, `overview` |

**API 参考模板参数示例：**

```json
{
  "api_name": "startAbility",
  "brief_description": "启动一个Ability实例",
  "since_version": "API 12",
  "declaration": "function startAbility(options: StartAbilityOptions): Promise<void>;",
  "parameters": [
    {"name": "options", "type": "StartAbilityOptions", "required": true, "description": "启动参数"}
  ],
  "return_type": "Promise<void>",
  "return_description": "无返回值",
  "error_codes": [
    {"id": "201", "description": "权限校验失败"}
  ],
  "permission": "ohos.permission.START_ABILITY",
  "restriction": "仅支持前台应用调用",
  "example_code": "await startAbility({...});"
}
```

### 3.3 `doc-solution build-kb` — 知识库构建

```bash
# 基本用法
doc-solution build-kb --input ./customer-inputs/ --name "华为-HarmonyOS"

# 参数说明
--input, -i   # 必填: 客户提供的源材料目录
--name, -n    # 必填: 客户名称
--output, -o  # 可选: 输出目录 (默认: knowledge)
--force       # 可选: 覆盖已有知识库
```

**输入目录结构建议：**

```
customer-inputs/
├── *.md              # 源文档 (API参考、开发指南等)
├── *.d.ts            # 接口声明文件
├── templates/        # 模板文件 (*.j2)
│   ├── api-ref.md.j2
│   └── dev-guide.md.j2
├── rules/            # 自定义规则 (可选)
└── checklist/        # 检查清单 (可选)
```

**构建产物：**

```
knowledge/
├── config.yaml           # 知识库配置
├── rules/                # 生成的规则集
│   ├── vale/.vale.ini    # Vale 配置
│   └── custom/           # 自定义规则
├── templates/            # 注册的模板
├── glossary/terms.yaml   # 提取的术语表
├── checklist/            # 检查清单
└── meta/style-profile.yaml  # 文档风格摘要
```

## 四、AI Agent 最佳实践

### 4.1 新客户接入

```yaml
步骤:
  1. 收集客户源材料 (文档+SDK+标准+模板)
  2. 调用 build-kb 构建知识库
  3. 查看 knowledge/config.yaml 确认配置正确
  4. 用 check 验证知识库规则是否准确
  5. 输出接入报告
```

### 4.2 内容开发

```yaml
步骤:
  1. 理解需求 (新增/修改/删除)
  2. 确定使用的模板
  3. 调用 generate 生成内容
  4. 检查生成的 check 报告
  5. 如有问题，调整参数重新生成
  6. 输出给客户人工审核
```

### 4.3 质量巡检

```yaml
步骤:
  1. 确定巡检范围 (目录/文件)
  2. 调用 check --check-type all
  3. 分析报告中的 error/warning
  4. 定位问题文件
  5. 修复 (手动或重新生成)
  6. 再次 check 验证
```

### 4.4 Vale 使用方式

Vale 是可选增强工具，本项目提供了两种使用方式：

**方式一：自动检测**（推荐）
系统会自动按以下顺序查找 Vale：
1. `--vale-bin` 参数指定的路径
2. 系统 PATH 环境变量
3. 项目捆绑包 `knowledge/vale.exe`

**方式二：指定捆绑包路径**
```bash
doc-solution check --target ./docs/ --vale-bin knowledge/vale.exe
```

**没有 Vale 时：**
- `check --check-type structure` 可独立运行
- 内置格式检查 (段落长度、代码块标注) 仍可用
- 风格检查 (terminology) 会跳过，不报错

## 五、项目自维护

当你需要维护本系统自身时，请遵循 `AGENTS.md` 中的规范。

关键点：
1. 修改后运行 `python -m pytest tests/ -v` 确保测试通过
2. 更新 `DEVELOPMENT_LOG.md`
3. 如影响客户使用，同步更新 `ROADMAP.md`
