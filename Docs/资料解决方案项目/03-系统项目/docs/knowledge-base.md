---
audience: ai-agent
priority: medium
purpose: 知识库构建、配置与使用指南
category: guide
last-updated: 2026-06-03
---

# 知识库指南

> 如何构建、配置和使用客户特定的知识库

---

## 概览

知识库（KB）是资料解决方案系统的**核心差异化功能**。它捕获客户特定的知识，包括文档风格、术语、模板、规则和检查清单，使自动化文档开发能够遵循客户规范。

> **关于知识库内容构建方法论**（如何编写 Vale 规则、提取术语、转换测试标准），请参见 `docs/kb-construction-guide.md`。本指南仅涵盖目录结构和注册流程。

## 知识库的构建方式

### 步骤 1：准备输入材料

将客户源材料收集到单个目录：

```
customer-inputs/
  |-- docs/                 # 源文档（.md）
  |   |-- api-reference.md
  |   |-- development-guide.md
  |-- templates/            # Jinja2 模板文件
  |   |-- api-ref/
  |   |   +-- template.md.j2
  |   +-- dev-guide/
  |       +-- template.md.j2
  |-- rules/                # 自定义规则（可选）
  |-- glossary/             # 术语定义（可选）
  +-- checklist/            # 质量检查清单（可选）
```

### 步骤 2：运行 build-kb

```bash
doc-solution build-kb --input ./customer-inputs/ --name "客户名称"
```

此命令：

1. **扫描**输入目录中的所有文件（.md、.yaml、.json、.py、.ts、.d.ts、.txt、.j2）
2. **分析**最多 50 个 .md 文件的文档风格：
   - 标题层级分布
   - 段落长度统计
   - 长段落比例
3. **生成** Vale 配置：
   - `.vale.ini`（含 StylesPath、MinAlertLevel）
   - `DocsStyle/HeadingHierarchy.yml` 规则
   - `Custom/Terminology.yml` 规则
4. **注册**输入中找到的模板
5. **写入**知识库配置到 `knowledge/config.yaml`

### 步骤 3：验证知识库

```bash
doc-solution check --target ./customer-inputs/docs/
```

检查时使用知识库的 Vale 配置来验证文档是否遵循客户风格。

## 知识库目录结构

构建完成后，知识库结构如下：

```
knowledge/
  |-- config.yaml                      # 主配置（AI Agent 可读）
  |-- rules/
  |   |-- vale/
  |   |   |-- .vale.ini                # Vale 配置
  |   |   +-- styles/
  |   |       |-- DocsStyle/
  |   |       |   +-- HeadingHierarchy.yml
  |   |       +-- Custom/
  |   |           +-- Terminology.yml
  |   +-- custom/
  |       |-- format-rules.yaml        # 自定义格式规则
  |       +-- structure-rules.yaml     # 自定义结构规则
  |-- templates/
  |   |-- api-ref/
  |   |   +-- template.md.j2
  |   +-- dev-guide/
  |       +-- template.md.j2
  |-- glossary/
  |   |-- terms.yaml                   # 术语定义
  |   +-- abbreviations.yaml           # 缩写列表
  |-- checklist/
  |   |-- quality-checklist.yaml       # 质量检查项
  |   +-- review-checklist.yaml        # 审查检查项
  +-- meta/
      +-- style-profile.yaml           # 文档风格分析
```

## 配置参考

### config.yaml

```yaml
# 示例生成的 config.yaml
customer:
  name: "Huawei-HarmonyOS"
  version: "1.0.0"
  created: "2026-06-02"

rules:
  vale:
    enabled: true
    config_path: "rules/vale/.vale.ini"
    styles_path: "rules/vale/styles/"
  custom:
    format_rules: "rules/custom/format-rules.yaml"
    structure_rules: "rules/custom/structure-rules.yaml"

templates:
  api-ref:
    path: "templates/api-ref/"
    primary: "template.md.j2"
  dev-guide:
    path: "templates/dev-guide/"
    primary: "template.md.j2"

glossary:
  terms_file: "glossary/terms.yaml"
  abbreviations_file: "glossary/abbreviations.yaml"

checklist:
  quality: "checklist/quality-checklist.yaml"
  review: "checklist/review-checklist.yaml"
```

### style-profile.yaml

```yaml
# 示例生成的风格档案
file_count: 15
analyzed_count: 15
heading_stats:
  average_per_file: 8.3
  level_distribution:
    1: 15
    2: 42
    3: 58
    4: 10
paragraph_stats:
  long_paragraph_ratio: 12.5
```

## 如何使用知识库

### 用于文档检查

运行 `check` 时自动使用知识库：

```bash
# 使用知识库的 Vale 配置和自定义规则
doc-solution check --target ./output-docs/
```

check 命令：
1. 读取 `knowledge/config.yaml` 获取知识库配置
2. 应用知识库的 Vale 配置进行风格/格式检查
3. 使用内置规则进行结构检查
4. 生成统一报告

### 用于内容生成

`generate` 命令使用知识库的模板：

```bash
# 使用知识库模板 'api-ref'
doc-solution generate --template api-ref --params '{...}'

# 使用知识库模板 'dev-guide'
doc-solution generate --template dev-guide --params '{...}'
```

### 用于自定义规则

`knowledge/rules/custom/` 中的自定义规则扩展了内置检查：

- **format-rules.yaml**：定义文档格式规范（标题风格、列表风格等）
- **structure-rules.yaml**：定义必需章节、文档结构模板

## 多客户支持

对于多个客户，维护独立的知识库目录并使用 `--output` 标志：

```bash
doc-solution build-kb --input ./customer-a/ --name "客户A" --output ./kb-customer-a/
doc-solution build-kb --input ./customer-b/ --name "客户B" --output ./kb-customer-b/
```

然后引用相应的知识库进行检查：

```bash
doc-solution check --target ./customer-a-docs/ --config ./kb-customer-a/rules/vale/.vale.ini
```

## 知识库维护

### 增量更新

```bash
doc-solution build-kb --input ./updated-docs/ --name "客户名称" --force
```

`--force` 标志会覆盖现有知识库。

### 手动编辑

所有知识库文件均为 YAML/JSON/纯文本。直接编辑即可：

- 向 `glossary/terms.yaml` 添加新术语
- 修改 `rules/vale/styles/` 中的 Vale 规则
- 向 `templates/` 添加新模板
- 更新 `checklist/` 中的质量检查清单
