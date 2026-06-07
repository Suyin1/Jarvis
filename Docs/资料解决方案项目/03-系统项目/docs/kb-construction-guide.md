---
audience: ai-agent
priority: high
purpose: AI Agent 从客户输入构建知识库内容的完整方法论
category: guide
last-updated: 2026-06-03
---

# 知识库构建指南

> AI Agent：这是你从客户输入材料构建知识库内容的逐步方法论。

---

## 概览

`build-kb` CLI 命令创建知识库的**目录结构和索引**。但**语义内容**（术语规则、Vale YAML 规则、术语表条目、检查清单项）需要由你（AI Agent）按照本指南中的方法论来编写。

### 本指南涵盖的内容

| 主题 | 章节 |
|------|------|
| 输入格式处理（md/xlsx/csv/json/yaml/d.ts/...） | [输入格式处理](#输入格式处理) |
| 完整的 5 步构建工作流 | [构建工作流](#构建工作流) |
| 术语和缩写提取 | [术语提取](#步骤-2术语提取) |
| Vale 规则 YAML 生成（全部 9 种类型） | [Vale 规则生成](#步骤-3vale-规则生成) |
| 测试标准转换为检查清单 | [测试标准转换](#步骤-4测试标准转换) |
| 使用 build-kb 注册 | [注册](#步骤-5使用-build-kb-注册) |
| 完整示例 | [端到端示例](#端到端示例) |

---

## 输入格式处理

客户输入材料有各种格式。下表说明如何处理每种格式：

| 格式 | 扩展名 | 处理策略 | 提取内容 |
|------|--------|----------|----------|
| Markdown | `.md` | 解析标题、代码块、段落、表格、列表 | 文档结构、术语、风格模式、API 签名 |
| Excel | `.xlsx` `.xls` | 将单元格内容作为文本读取；按工作表/列读取 | 表格通常是参数规格、错误码列表、术语对（A 列=术语，B 列=定义）、测试用例 |
| CSV | `.csv` | 解析行和列 | 类似 xlsx：参数表、术语列表、测试用例矩阵 |
| JSON | `.json` | 展平嵌套结构，键名是术语 | API Schema、配置规格、枚举值、属性名 |
| YAML | `.yaml` `.yml` | 解析为结构化数据，键名是术语 | 配置规格、规则定义、枚举值 |
| TypeScript | `.ts` `.d.ts` | 提取接口/类型/枚举名、函数签名、参数名、注释 | 类型定义、API 接口、枚举字面量、JSDoc 注释（包含术语描述） |
| Python | `.py` | 提取函数签名、类名、文档字符串 | API 定义、参数名、文档字符串风格规格 |
| Jinja2 | `.j2` | 解析模板变量 `{{ var }}` 和 `{% %}` 块 | 模板结构、变量命名规范 |
| 纯文本 | `.txt` | 逐行读取 | 简单规格、列表 |

### Excel/CSV 处理规则

处理电子表格时，应用以下启发式规则：

```yaml
工作表命名：
  - "术语表" / "术语" / "Glossary" / "Terminology": 术语定义
  - "参数" / "Parameters" / "API" / "接口": API 参数规格
  - "错误码" / "Error Codes": 错误码定义
  - "测试用例" / "Test Cases": 测试用例规格
  - "模板" / "Templates": 模板规格

列检测：
  - A 列 = 术语/名称，B 列 = 定义/说明（约定俗成）
  - 表头：查找包含已知关键词（term、name、名称、参数名等）的第一行
  - 跳过关键列为空的行
```

---

## 构建工作流

```
+-----------+     +-----------+     +-----------+     +-----------+     +-----------+
| 步骤 1    | --> | 步骤 2    | --> | 步骤 3    | --> | 步骤 4    | --> | 步骤 5    |
| 结构分析   |     | 术语提取   |     | Vale      |     | 测试标准   |     | 使用      |
|           |     |           |     | 规则生成   |     | 转换      |     | build-kb  |
+-----------+     +-----------+     +-----------+     +-----------+     +-----------+
     |                 |                 |                 |                 |
     v                 v                 v                 v                 v
 style-profile     glossary/         rules/vale/        checklist/        config.yaml
 (手动)             terms.yaml        styles/*.yml       *.yaml            (通过 build-kb)
```

---

## 步骤 1：文档结构分析

### 分析内容

分析客户的 `.md` 文档并记录：

```yaml
# 需要回答的问题：
- 他们使用什么标题层级模式？（H1 > H2 > H2，还是 H1 > H3？）
- 典型的文档模板是什么？（章节按什么顺序排列？）
- 代码块如何标注？（```typescript、```java，还是无标注？）
- 典型段落长度是多少？（简短要点还是长段落？）
- 使用了哪些表格？（参数表、比较表？）
- 是否有每个文档都必须包含的必需章节？
```

### 输出：更新 style-profile.yaml

编辑 `knowledge/meta/style-profile.yaml`，写入分析结果：

```yaml
file_count: 15
analyzed_count: 15
heading_stats:
  average_per_file: 8.3
  level_distribution:
    1: 15
    2: 42
    3: 58
    4: 10
  # 手动添加以下内容：
  typical_pattern: "H1 > H2 > H3"        # 最常见的标题路径
  has_intro_section: true                  # 文档通常以介绍开头
has_required_sections:                     # 每个文档必须包含的章节
  - "API 参考"
  - "参数"
  - "错误码"

paragraph_stats:
  long_paragraph_ratio: 12.5
  typical_style: "bullet-point-heavy"      # 或 "prose-heavy"

code_block_patterns:
  - language: "typescript"                 # 最常见的代码块语言
    frequency: 80%
  - language: "java"
    frequency: 15%

table_patterns:
  - style: "parameter-table"               # 名称 | 类型 | 必需 | 说明
    columns: ["name", "type", "required", "description"]
```

### 模式检测指南

| 观察到的情况 | 可能的规范 |
|-------------|-----------|
| 80%+ 的代码块使用 ```typescript | 要求语言标注 |
| 所有文档都有"API 参考"+"错误码" | 标记为必需章节 |
| "page"和"Page"混用 | 添加术语一致性规则 |
| H1 > H2 > H4（跳过 H3） | 可能是有意为之；或标记为警告 |
| 要点平均 50 个字符 | 可接受；无段落长度问题 |

---

## 步骤 2：术语提取

### 提取来源

从所有输入格式中提取术语，不仅限于 `.md`：

| 来源 | 查找位置 | 提取内容 |
|------|----------|----------|
| `.md` 文档 | 重复术语、标题、代码注释 | 领域术语、API 名称、概念 |
| `.ts` / `.d.ts` | 接口名、类型名、枚举值、JSDoc `@param` | 类型名、枚举字面量、参数名 |
| `.xlsx` 术语表 | A/B 列对 | 权威术语定义 |
| `.json` / `.yaml` 配置 | 键名、枚举数组、描述字段 | 配置术语、允许值 |
| `.csv` 术语列表 | 直接的术语/定义对 | 术语表条目 |
| 模板 `.j2` | 模板变量名 | 模板参数命名规范 |

### 输出：glossary/terms.yaml

```yaml
# 从客户文档中提取的标准术语
startAbility:
  zh: "启动Ability"
  en: "startAbility"
  category: "api"
  notes: "功能激活的主要入口点"

FA:
  zh: "功能适配"
  en: "Feature Adaptation"
  category: "abbreviation"
  preferred: true                       # 在文档中优先于全称使用

page:
  zh: "页面"
  en: "page"
  category: "common"
  alternatives:                          # 应避免的术语
    - "Page"
    - "界面"

# 从 .d.ts 枚举值提取
AbilityType:
  zh: "Ability类型"
  en: "AbilityType"
  category: "type"
  values:                                # 枚举字面量值
    - "FEATURE"
    - "PAGE"
    - "SERVICE"
```

### 输出：glossary/abbreviations.yaml

```yaml
# 在客户材料中发现的缩写
FA: "Feature Adaptation"
UI: "User Interface"
API: "Application Programming Interface"
SDK: "Software Development Kit"
MCP: "Model Context Protocol"
```

### 提取启发式规则

```yaml
启发式规则 1 - 重复大写模式：
  如果：某个词在多个文档中始终全大写
  那么：可能是缩写；添加到 abbreviations.yaml

启发式规则 2 - 代码与文本不匹配：
  如果：代码使用 "startAbility" 但文档使用 "start ability" 或 "start_ability"
  那么：添加术语规则，强制使用 "startAbility"

启发式规则 3 - 电子表格中的术语表：
  如果：xlsx 包含列 [术语, 说明] 或 [Term, Definition]
  那么：每行是一个术语表条目

启发式规则 4 - .d.ts 中的类型/接口：
  如果：接口/类型名出现在 JSDoc @param 中
  那么：这是一个值得添加到术语表的领域类型
```

---

## 步骤 3：Vale 规则生成

### 规则类型决策树

```
你需要什么类型的检查？
  |
  +-- "不要使用单词 X" → existence
  +-- "使用 X，不要用 Y" → substitution
  +-- "最多 N 次" → occurrence
  +-- "无重复词" → repetition
  +-- "统一风格" → consistency
  +-- "如果 X 则 Y" → conditional
  +-- "必须大写" → capitalization
  +-- "自定义词典" → spelling
  +-- "可读性" → metric
```

### 规则类型模板

#### existence — 检查是否包含某些术语

使用场景：需要禁止或要求特定词语。

```yaml
# 场景：禁止使用"please"、"easy"等非技术用语
# 文件：rules/vale/styles/Custom/NoPlease.yml
extends: existence
message: "避免使用 '%s'，使用更精确的技术描述"
level: warning
ignorecase: true
scope: text
tokens:
  - 'please'
  - 'easily'
  - 'simply'
  - 'just'
```

#### substitution — 替换不正确的术语

使用场景：客户有标准术语，必须一致使用。

```yaml
# 场景："拉起" → "启动", "page" → "页面"
# 文件：rules/vale/styles/Custom/TermSubstitution.yml
extends: substitution
message: "建议使用 '%s' 代替 '%s'"
level: error
ignorecase: true
swap:
  "拉起": "启动"
  "start up": "startUp"
  "page": "页面"
  "点击": "单击"
```

#### occurrence — 限制术语频率

使用场景：某个术语每页最多出现 N 次。

```yaml
# 场景："注意" 每页不超过 3 次
# 文件：rules/vale/styles/Custom/NoteOccurrence.yml
extends: occurrence
message: "本页出现 '%s' %d 次，建议不超过 3 次"
level: suggestion
scope: paragraph
max: 3
tokens:
  - '注意'
  - 'Note:'
```

#### repetition — 检测重复词

使用场景：需要捕获意外的词重复。

```yaml
# 场景：检测重复词 "the the"
# 文件：rules/vale/styles/Custom/RepeatedWords.yml
extends: repetition
message: "'%s' 重复出现"
level: warning
ignorecase: true
tokens:
  - 'the'
  - 'a'
  - 'an'
  - 'to'
  - 'is'
  - 'in'
  - 'on'
  - 'at'
```

#### consistency — 强制一致使用

使用场景：同一概念的两种形式被混用。

```yaml
# 场景："e-mail" 与 "email" 必须统一
# 文件：rules/vale/styles/Custom/EmailConsistency.yml
extends: consistency
message: "术语不一致: 同时使用了 '%s' 和 '%s'"
level: warning
ignorecase: true
either:
  - 'e-mail'
  - 'email'
```

#### conditional — 如果 X 则检查 Y

使用场景：使用某个术语意味着也应使用另一术语。

```yaml
# 场景：如果出现 "click"，也要检查是否有用户操作指导
# 文件：rules/vale/styles/Custom/ClickConditional.yml
extends: conditional
message: "使用了 'click'，建议补充完整的用户操作路径"
level: suggestion
ignorecase: true
first: '\bclick\b'
second: '(click|tap|select|choose|press)'
```

#### capitalization — 检查大小写模式

使用场景：产品名称或术语有特定的大小写规则。

```yaml
# 场景："startAbility" 而不是 "startability" 或 "StartAbility"
# 文件：rules/vale/styles/Custom/CamelCaseTerms.yml
extends: capitalization
message: "'%s' 应使用小写驼峰"
level: error
ignorecase: false
match: $sentence
style: camelCase
indicators:
  - 'startAbility'
  - 'onForeground'
  - 'createModule'
```

#### spelling — 自定义拼写检查

使用场景：领域特定术语被 Vale 的默认词典标记为拼写错误。

```yaml
# 场景：添加行业术语到 Vale 拼写检查
# 这不是 YAML 规则文件。创建纯文本单词列表即可。
# 文件：rules/vale/styles/Custom/vocab.txt
startAbility
AbilityType
onForeground
Lifecycle
HarmonyOS
```

然后在 `.vale.ini` 中引用：
```ini
[*.md]
Vale.Spelling = YES
Vale.Spelling.vocab = Custom
```

#### metric — 可读性指标

使用场景：文档必须达到最低可读性分数。

```yaml
# 场景：要求文档 Flesch 阅读分数 >= 60
# 文件：rules/vale/styles/DocsStyle/Readability.yml
extends: metric
message: "Flesch 阅读分数 %s (建议 >= 60)"
level: suggestion
metrics:
  - FleschReadingEase
  - FleschKincaidGrade
score: 60
```

### 如何选择严重级别

| 严重级别 | 使用场景 |
|----------|----------|
| `error` | 必须精确的术语（API 名称、类型名） |
| `warning` | 应遵循的风格规范 |
| `suggestion` | 最佳实践、可读性改进 |

### 如何选择作用域

| 作用域 | 应用于 | 示例 |
|--------|--------|------|
| `text` | 所有文本内容 | 通用术语 |
| `heading` | 仅标题行 | 标题格式规则 |
| `code` | 仅代码块 | 代码风格规则 |
| `table` | 表格单元格 | 表格格式规则 |
| `paragraph` | 段落级别 | 段落重复 |
| `sentence` | 句子级别 | 句子长度 |

---

## 步骤 3.5：规则测试

创建 Vale 规则后，必须验证其正确性。系统提供了一个**通用测试工具** — `doc-solution test-rule` — 适用于任何 Vale 规则类型，不关心具体检查内容。

### 测试工具的工作原理

```
test-rule --rule <规则.yml> --should-fail <失败.md> --should-pass <通过.md>
         |
         v
+----------+--------+    +-------------------+    +-------------------+
| 语法检查           |--->| 正向测试           |--->| 负向测试           |
| (Vale 接受 YAML?)  |    | (should-fail 文档  |    | (should-pass 文档  |
|                    |    |  触发告警)         |    |  无告警)           |
+-------------------+    +-------------------+    +-------------------+
         |                      |                       |
         v                      v                       v
   valid_syntax          positive_test            negative_test
   = True/False          = True/False/None        = True/False/None
```

执行三项检查：

| 检查 | 验证内容 | 所需输入 |
|------|---------|----------|
| 语法 | Vale 无错误地接受 YAML | 仅规则文件 |
| 正向 | 正确检测违规 | 规则 + should-fail 文档 |
| 负向 | 干净内容上无误报 | 规则 + should-pass 文档 |

**这是通用的** — 测试工具不关心也不了解规则检查什么。它只验证：
- Vale 能解析该规则（语法）
- should-fail 文档产生告警（正向）
- should-pass 文档产生零告警（负向）

### CLI 参考

```bash
# 基本语法验证（仅规则文件）
doc-solution test-rule --rule rules/vale/styles/Custom/TermSubstitution.yml

# 完整测试（正向 + 负向）
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --should-pass ./tests/should-pass.md

# JSON 输出（用于程序化使用）
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --output json
```

### 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 通过 — 所有测试通过 |
| 1 | 失败 — 正向或负向测试未通过 |
| 1 | 语法错误 — 规则文件存在语法错误 |

### 如何创建测试文档

为每条创建的规则，必须创建两个测试文档：

**should-fail.md** — 包含应触发规则的内容。

```markdown
# 测试 - 应失败

本文档故意使用不正确的术语。
api 应该大写为 API。
sdk 应该大写为 SDK。
```

**should-pass.md** — 包含不应触发规则的内容。

```markdown
# 测试 - 应通过

本文档使用正确的术语。
API 已正确大写。
SDK 已正确大写。
```

### 测试文档构建规则

创建测试文档时，遵循以下原则：

```yaml
规则："任意类型的 Vale 规则（existence、substitution 等）"

should-fail.md 构建：
  1. 至少包含一个明显违反规则的实例
  2. 使违规行为显而易见（如确切的错误术语）
  3. 周围包含正常文本以确认作用域检测有效

should-pass.md 构建：
  1. 包含规则检查的所有内容的正确/规范形式
  2. 添加近似匹配以验证无误报
     （如规则检查"api" -> should-pass 包含"API"和"APIs"）
  3. 包含最少的上下文

需要覆盖的边界情况：
  - 大小写敏感：如果规则设置了 ignorecase: true，测试两种形式
  - 部分匹配："API_KEY"不应触发关于"API"的规则
  - 代码块：取决于作用域，代码块可能被排除
  - 标题：如果作用域排除标题，验证标题无告警
```

### 完整示例

**规则文件**（`rules/vale/styles/Custom/ProductName.yml`）：
```yaml
extends: substitution
message: "使用 '%s' 替代 '%s'"
level: error
ignorecase: true
swap:
  "鸿蒙": "HarmonyOS"
  "harmonyos": "HarmonyOS"
```

**should-fail.md**（`tests/ProductName/should-fail.md`）：
```markdown
# 测试 - 应失败

本指南涵盖在鸿蒙上的开发。
harmonyos SDK 提供了 API。
```

**should-pass.md**（`tests/ProductName/should-pass.md`）：
```markdown
# 测试 - 应通过

本指南涵盖在 HarmonyOS 上的开发。
HarmonyOS SDK 提供了 API。
```

**运行测试：**
```bash
doc-solution test-rule \
    --rule rules/vale/styles/Custom/ProductName.yml \
    --should-fail tests/ProductName/should-fail.md \
    --should-pass tests/ProductName/should-pass.md
```

**预期输出：**
```
规则:     Custom.ProductName
文件:     rules/vale/styles/Custom/ProductName.yml
摘要:     通过

[语法]
  有效:   是

[正向测试]
  结果:   通过 — 规则捕获了违规
  告警:   [error] 使用 'HarmonyOS' 替代 '鸿蒙'
  告警:   [error] 使用 'HarmonyOS' 替代 'harmonyos'

[负向测试]
  结果:   通过 — 无误报
```

### 集成到工作流

在创建规则（步骤 3）之后、处理测试标准（步骤 4）之前：

```
步骤 3: 创建 Vale 规则 YAML 文件
   |
   v
步骤 3.5: 使用 test-rule 测试每条规则
   |  - 创建 should-fail.md 和 should-pass.md
   |  - 为每条规则运行 test-rule
   |  - 修复规则直至全部通过
   |  - 将测试文件包含在 customer-inputs/ 中以保持可重现
   v
步骤 4: 测试标准转换
```

### 可重复的测试套件

为便于维护，将测试文件放在规则旁边：

```
customer-inputs/
  |-- rules/vale/styles/Custom/
  |   |-- ProductName.yml
  |   +-- TermSubstitution.yml
  |-- tests/
  |   |-- ProductName/
  |   |   |-- should-fail.md
  |   |   +-- should-pass.md
  |   +-- TermSubstitution/
  |       |-- should-fail.md
  |       +-- should-pass.md
  +-- ...
```

这允许在修改规则时重新运行所有规则测试：

```bash
for rule in rules/vale/styles/Custom/*.yml; do
  name=$(basename "$rule" .yml)
  fail="tests/$name/should-fail.md"
  pass="tests/$name/should-pass.md"
  if [ -f "$fail" ] && [ -f "$pass" ]; then
    doc-solution test-rule --rule "$rule" --should-fail "$fail" --should-pass "$pass"
  fi
done
```

---

## 步骤 4：测试标准转换

### 输入类型和转换策略

| 输入格式 | 典型内容 | 转换为检查清单 |
|----------|----------|---------------|
| `.xlsx` 测试用例表 | 测试用例 ID、步骤、预期结果、前置条件 | 每个测试用例 → 检查项（基于场景） |
| `.md` 测试规格 | 测试场景、环境搭建、验收标准 | 验收标准 → 检查项 |
| `.yaml`/.json 测试配置 | 测试参数、允许值、边界值 | 验证规则 → 检查项 |
| `.csv` 测试矩阵 | 参数和预期输出的组合 | 参数规则 → 检查项 |
| `.d.ts` 类型定义 | 接口形状、可选/必填字段、枚举值 | 必填字段 → 检查项 |

### 转换方法论

对于每个测试标准输入，遵循以下流程：

```yaml
步骤 1：确定测试范围
  - 测试的是哪个功能/模块？
  - 输入格式是什么？（xlsx 表格？md 场景列表？）

步骤 2：提取检查点
  - 前置条件 → "文档必须提及前置条件"
  - 步骤 → "文档必须按正确顺序覆盖所有步骤"
  - 预期结果 → "文档必须说明预期结果"
  - 边界情况 → "文档必须覆盖错误场景"
  - 边界值 → "文档必须说明参数限制"

步骤 3：分类检查类型
  - "auto"：可自动化（Vale 规则或内置检查）
  - "ai-review"：需要 AI Agent 审查（主观）
  - "manual"：需要人工审查（依赖上下文）

步骤 4：写入检查清单 YAML
```

### 示例：转换测试规格

**输入（`test-cases.xlsx`）：**

| 测试用例 | 场景 | 步骤 | 预期 |
|---------|------|------|------|
| TC-001 | 启动 ability | 1. 调用 startAbility() 2. 验证结果 | Ability 已启动 |
| TC-002 | 传入空参数 | 调用 startAbility(null) | 返回错误码 400 |
| TC-003 | 后台启动 | 应用后台时调用 startAbility() | 排队，前台时启动 |

**输出（`checklist/quality-checklist.yaml`）：**

```yaml
checklist:
  - id: "tc-001-coverage"
    name: "正常启动场景 - 文档必须包含 startAbility 的基本调用步骤"
    source: "TC-001"
    type: "auto"
    severity: "error"

  - id: "tc-002-coverage"
    name: "异常参数场景 - 文档必须说明空参数的处理方式"
    source: "TC-002"
    type: "ai-review"
    severity: "warning"

  - id: "tc-003-coverage"
    name: "后台启动场景 - 文档必须说明后台状态下的行为"
    source: "TC-003"
    type: "ai-review"
    severity: "suggestion"

  - id: "param-boundary"
    name: "参数边界值测试 - 每个参数需注明取值范围"
    source: "extracted-from-param-tables"
    type: "auto"
    severity: "error"
```

### 示例：转换类型定义（.d.ts）

**输入（`api.d.ts`）：**

```typescript
interface StartAbilityOptions {
  abilityName: string;       // 必填
  startMode?: StartMode;     // 可选
  timeout: number;           // 必填，毫秒
}

enum StartMode {
  FOREGROUND = "foreground",
  BACKGROUND = "background",
}
```

**输出：**

```yaml
# checklist/review-checklist.yaml 补充：
  - id: "dts-param-coverage"
    name: "所有必需参数(abilityName、timeout)必须在文档中列出"
    source: "api.d.ts:StartAbilityOptions"
    type: "auto"
    severity: "error"

  - id: "dts-enum-values"
    name: "枚举值(FOREGROUND/BACKGROUND)必须在文档中解释"
    source: "api.d.ts:StartMode"
    type: "ai-review"
    severity: "suggestion"
```

```yaml
# glossary/terms.yaml 补充：
StartMode.FOREGROUND:
  zh: "前台启动"
  en: "FOREGROUND"
  category: "enum-value"
  source: "api.d.ts"
```

---

## 步骤 5：使用 build-kb 注册

在步骤 1-4 中创建所有内容后，使用 `build-kb` 进行注册：

```bash
# 步骤 5a：将所有文件放在一个输入目录中
customer-inputs/
  |-- rules/vale/styles/Custom/
  |   |-- NoPlease.yml            # （来自步骤 3）
  |   |-- TermSubstitution.yml    # （来自步骤 3）
  |   +-- ...
  |-- glossary/
  |   |-- terms.yaml              # （来自步骤 2）
  |   +-- abbreviations.yaml      # （来自步骤 2）
  |-- checklist/
  |   +-- quality-checklist.yaml  # （来自步骤 4）
  |-- templates/
  |   +-- api-ref/
  |       +-- template.md.j2      # （客户提供）
  +-- meta/
      +-- style-profile.yaml      # （来自步骤 1）

# 步骤 5b：运行 build-kb 进行注册
doc-solution build-kb --input ./customer-inputs/ --name "客户名称" --force
```

### build-kb 的实际作用

| 操作 | 产出 |
|------|------|
| 创建目录 | `rules/vale/styles/DocsStyle/`、`rules/custom/` 等 |
| 扫描 `.md` 文件 | style-profile.yaml（标题/段落统计） |
| 生成 2 条默认 Vale 规则 |（可被自定义文件覆盖）|
| 扫描 `.j2` 文件 | config.yaml 中的模板条目 |
| 生成 config.yaml | 所有知识库资源的索引 |

**你的自定义文件（Vale 规则、术语表、检查清单）保持原样。** `build-kb` 命令只是将其复制到输出结构中并创建索引。

---

## 端到端示例

### 收到的客户输入

```
customer-inputs/
  |-- docs/
  |   |-- api-reference.md         # 主要 API 文档
  |   +-- development-guide.md     # 开发指南
  |-- specs/
  |   |-- api-definitions.d.ts     # TypeScript 类型定义
  |   |-- test-cases.xlsx          # 测试用例矩阵
  |   +-- terminology.xlsx         # 术语定义（A 列=术语，B 列=描述）
  |-- templates/
  |   +-- api-ref/
  |       +-- template.md.j2       # 现有的 Jinja2 模板
```

### AI Agent 构建过程

```yaml
步骤 1 - 文档分析：
  读取：api-reference.md、development-guide.md
  观察：
    - 一致地使用 H1 > H2 > H3 模式
    - 代码块总是标注 ```typescript
    - 常见术语：startAbility、onForeground、AbilityType
    - 必需章节："API 参考"、"参数"、"错误码"

步骤 2 - 术语提取：
  从 api-definitions.d.ts：
    - 接口：StartAbilityOptions { abilityName、startMode、timeout }
    - 枚举：StartMode { FOREGROUND、BACKGROUND }
  从 terminology.xlsx：
    - FA → Feature Adaptation
    - MCP → Model Context Protocol
  写入：glossary/terms.yaml + abbreviations.yaml

步骤 3 - Vale 规则生成：
  创建规则：
    - Custom/TermSubstitution.yml（强制驼峰 API 名称）
    - Custom/NoPlease.yml（禁止非正式语言）
    - DocsStyle/HeadingHierarchy.yml（按需调整）

步骤 3.5 - 规则测试：
  为每条规则创建测试文档并运行 test-rule：
    - tests/TermSubstitution/should-fail.md + should-pass.md
    - tests/TermSubstitution/ -> doc-solution test-rule --rule ...（通过）
    - tests/NoPlease/ -> doc-solution test-rule --rule ...（通过）
  将测试文档包含在 customer-inputs/tests/ 中以保持可重现

步骤 4 - 测试标准转换：
  从 test-cases.xlsx：
    - TC-001：正常启动 → 自动检查项
    - TC-002：空参数 → ai-review 检查项
  写入：checklist/quality-checklist.yaml

步骤 5 - 注册：
  运行：doc-solution build-kb --input ./customer-inputs/ --name "Huawei-HarmonyOS" --force
  验证：doc-solution check --target ./customer-inputs/docs/
```

---

## 附录：完整 Vale 规则参考

| 规则类型 | YAML 关键字 | 必填字段 | 使用场景 |
|----------|------------|----------|----------|
| existence | `extends: existence` | `tokens` | 禁止/要求词语 |
| substitution | `extends: substitution` | `swap`（键值对） | 术语替换 |
| occurrence | `extends: occurrence` | `max`、`tokens` | 限制频率 |
| repetition | `extends: repetition` | `tokens` | 捕获重复 |
| consistency | `extends: consistency` | `either`（2 项以上列表） | 强制统一风格 |
| conditional | `extends: conditional` | `first`、`second` | X 蕴含 Y 检查 |
| capitalization | `extends: capitalization` | `match`、`style`、`indicators` | 大小写强制 |
| spelling |（词汇文件，非 YAML）| `.txt` 单词列表 | 自定义词典 |
| metric | `extends: metric` | `metrics`、`score` | 可读性分数 |

### 通用 YAML 字段（所有规则类型）

| 字段 | 类型 | 说明 |
|------|------|------|
| `message` | string | 向用户显示的错误信息。`%s` = 匹配的标记 |
| `level` | enum | `error`、`warning`、`suggestion` |
| `scope` | enum | `text`、`heading`、`code`、`table`、`paragraph`、`sentence` |
| `ignorecase` | bool | 不区分大小写匹配 |
| `action` | object | 自动修复配置（名称 + 参数） |

---

## 附录：build-kb 参考指南

```bash
# 构建新知识库
doc-solution build-kb --input ./customer-inputs/ --name "客户名称"

# 重建（覆盖现有的）
doc-solution build-kb --input ./customer-inputs/ --name "客户名称" --force

# 自定义输出路径
doc-solution build-kb --input ./customer-inputs/ --name "客户名称" --output ./kb-custom/

# --input 目录结构应为：
# customer-inputs/
#   |-- docs/           （用于风格分析的源 .md 文件）
#   |-- templates/      （.j2 Jinja2 模板）
#   |-- rules/          （可选：Vale YAML 规则）
#   |-- glossary/       （可选：terms.yaml、abbreviations.yaml）
#   |-- checklist/      （可选：质量检查清单 YAML）
#   +-- meta/           （可选：style-profile.yaml）
```
