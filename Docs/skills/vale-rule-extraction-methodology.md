---
audience: ai-agent
category: guide
priority: high
purpose: 从任意格式的客户材料中提取规范片段，转换为任意形式的可执行检查项
last-updated: 2026-06-09
---

# 规范片段提取与转换方法论

> 从客户提供的材料（格式不限）中提取可程序化检查的规范片段，
> 转换为适合目标系统的规则/检查/测试。

---

## 核心理念

**输入格式、输出形式、检测手段都是可变的。** 方法论本身不绑定任何格式或工具。

材料中隐藏着 5 种规范片段。目标是把它们找出来，再决定怎么用：

```
客户材料（任意格式）
  │
  ├─ md 规范文档
  ├─ xlsx 测试表格
  ├─ d.ts 类型定义
  ├─ json/yaml 配置
  ├─ csv 术语表
  ├─ 模板文件
  └─ 纯文本
        │
        ▼
规范片段提取（5 类）
  ├─ 禁止项  — "不能做什么"
  ├─ 要求项  — "必须做什么"
  ├─ 术语项  — "用哪个词"
  ├─ 结构项  — "按什么顺序/结构组织"
  └─ 测试项  — "输入什么→期望什么输出"
        │
        ▼
按需选择输出形式
  ├─ Vale .yml 规则
  ├─ Python 检查器
  ├─ glossary/terms.yaml
  ├─ checklist/*.yaml
  ├─ pytest 测试用例
  └─ 其他
```

---

## Step 1: 理解材料结构

不管格式是什么，先回答：

```
这份材料的组织形式是什么？
  逐条列举？ → 每条是一个规范片段
  表格结构？ → 每行是一个规范片段，列是属性
  层级嵌套？ → 按层级分解为规范片段
  自由文本？ → 逐句／逐段提取

材料中"正确"和"错误"是如何表达的？
  有明确示例？ → 对比正误示例提取差异
  有明确规则陈述？ → 直接提取规则语句
  有隐含规则？ → 从行文模式推断
```

### 按格式的处理策略

| 输入格式 | 规范片段通常藏在 | 提取方式 |
|----------|-----------------|---------|
| .md 文档 | `## 标准格式` / `## 问题现象` / `## 报错信息` | 按标题分组，对比正误示例 |
| .xlsx 表格 | 每一行是一个测试用例（输入列 + 期望输出列） | 行 = 规范片段，列 = 属性 |
| .d.ts / .ts | 接口/类型/枚举名 + JSDoc 注释 | 每个类型定义 = 一个术语项 |
| .json / .yaml | 键名、枚举值、描述字段 | 键 = 术语，值 = 定义 |
| .csv | 每行一对（术语, 定义）或（输入, 输出） | 直接解析行列 |
| .j2 模板 | `{% %}` `{{ }}` 标记的变量 | 变量名 = 术语项，模板结构 = 结构项 |
| .py | 函数签名 + docstring | 函数 = API 术语，参数 = 结构项 |

---

## Step 2: 分类与结构化

对提取出的每个片段，打上分类标签：

### 禁止项（Prohibition）

```
特征: "不能/不要/禁止使用 X", "X 是不允许的"
判断: 出现→告警
来源示例:
  md: "示例代码中不可使用console.log"
  xlsx: 列标题="禁止项", 行="制表符"
输出示例:
  Vale: existence 规则
  Python: if pattern in text: report()
```

### 要求项（Requirement）

```
特征: "必须/需要 X", "X 应满足 Y 条件"
判断: 不出现→告警, 或条件不满足→告警
来源示例:
  md: "每一行需要以|开头结尾"
  xlsx: 列标题="强制要求"
输出示例:
  Vale: existence（必须出现的模式）
  Python: 检查结构/存在性
```

### 术语项（Terminology）

```
特征: "应使用 X 而非 Y", "X 是标准写法"
判断: 出现 Y→告警（推荐 X）
来源示例:
  md: "使用『API』而不是『api』"
  xlsx: A列=错误写法, B列=正确写法
输出示例:
  Vale: substitution 规则
  glossary/terms.yaml 条目
```

### 结构项（Structure）

```
特征: "必须先写 X, 再写 Y", "文档必须包含 Z 章节"
判断: 结构顺序错误→告警, 缺少章节→告警
来源示例:
  md: "标题行、分隔符行、数据行的列数一致"
输出示例:
  Python 检查器（需要文档结构感知）
  checklist/*.yaml
```

### 测试项（Test Spec）

```
特征: "在 X 条件下, 应得到 Y 结果"
判断: 运行测试用例, 对比实际与期望
来源示例:
  xlsx: 输入列 + 期望输出列
  md: 正例/反例对比
输出示例:
  程序化测试（pytest、shell 脚本）
  should-pass.md / should-fail.md
```

---

## Step 3: 选择实现方式

分类之后，根据分类和可用工具选择实现方式：

```
禁止项 ─┬─ 简单模式匹配 → Vale existence
        ├─ 复杂模式 → Python regex
        └─ 需结构上下文 → Python AST 检查器

要求项 ─┬─ 存在性检查 → Vale existence
        ├─ 条件检查 → Vale conditional
        ├─ 数量检查 → Vale occurrence
        └─ 结构检查 → Python 检查器

术语项 ─┬─ 一对一词替换 → Vale substitution
        ├─ 大小写规范 → Vale capitalization
        ├─ 拼写校正 → Vale spelling
        └─ 术语表 → glossary/terms.yaml

结构项 ─┬─ 标题层级 → Vale heading scope
        ├─ 章节顺序 → Python 检查器
        ├─ 必需章节 → checklist
        └─ 表格结构 → Python 解析器

测试项 ─┬─ 正/反测试 → should-pass.md + should-fail.md
        ├─ 参数化测试 → pytest
        └─ 自动化验证 → shell 脚本
```

---

## Step 4: 测试验证

每种输出形式有对应的验证方式：

| 输出形式 | 验证方式 | 验证内容 |
|---------|---------|---------|
| Vale .yml | `test-rule --should-fail --should-pass` | 语法正确、正向触发、反向静默 |
| Python 检查器 | `pytest tests/` | 正例通过、反例告警 |
| glossary/terms.yaml | 手动审查 + 集成测试 | 术语完整、定义准确 |
| checklist .yaml | 对比文档检查 | 覆盖所有必需项 |
| pytest 用例 | `pytest tests/` | 输入→期望输出一致 |

---

## Step 5: 集成与注册

将产出的所有检查和规则注册到知识库：

```
oh-kb/
  ├─ rules/vale/styles/    ← Vale .yml 规则
  ├─ glossary/             ← 术语表 terms.yaml
  ├─ checklist/            ← 检查清单
  ├─ config.yaml           ← 注册规则路径
  └─ meta/                 ← 风格元数据
```

---

## 附录: Vale 类型实际可用性（vale version master 构建）

从 6 条新规则的实践经验看，Vale 的 9 种类型并非全部可用：

| Vale 类型 | 实际可用性 | 已知问题 |
|-----------|-----------|---------|
| **existence** | ✅ 完全可用 | 最稳定的类型，支持 `scope: raw` 检测代码块 |
| **substitution** | ✅ 完全可用 | 标准替换规则 |
| **occurrence** | ⚠️ 可用但有限制 | `max: 0` 不会生效（源码 `if a.Max > 0` 排除了 0）；`nonword` 不合法 |
| **consistency** | ❌ CJK 不可用 | Go RE2 不匹配任何 CJK（中文）字符 |
| **conditional** | ❌ 有 bug | `first` 永远触发，无视 `second` 是否匹配 |
| **metric** | ❌ 不支持 | 报 `"empty expression"` 错误 |
| **capitalization** | ❌ 未测试 | 需验证 |
| **repetition** | ❌ 未测试 | 需验证 |
| **spelling** | ❌ 未测试 | 需验证 |

### Vale Markdown 解析器已知限制

以下字符在 Vale 的 Markdown 解析中被剥离，**无法被任何 token 匹配**：

| 字符 | 原因 | 影响 |
|------|------|------|
| `!` (U+0021) | Markdown 图片语法 `![alt]` 标记 | 感叹号检测不可行 |
| `{` / `}` | Markdown 属性/脚注语法 | 无法检测 `try {`，code fence 语言标签等 |

### 建议

1. **首选 existence 类型** — 最稳定、最可预测
2. **CJK 规则 → Python 检查器** — Vale 不支持中文字符匹配
3. **需全局上下文规则 → Python 检查器** — 如 try-without-catch、句子长度等
4. **测试验证** — 始终使用 `test-rule --should-fail --should-pass` 验证规则实际行为

## 案例 1: 从 md 规范提取（OpenHarmony md-style-check）

```
输入: md-style-check.md（Markdown 规范文档）
格式: 章节式（## 标题 + 标准格式 + 问题现象 + 报错信息）

提取出的规范片段（示例）:
  禁止项: console.log() → 转为 Vale NoConsoleLog.yml (existence)
  要求项: 标题不以序号开头 → 转为 Vale HeadingNumbering.yml (existence)
  要求项: 表格行以|开头结尾 → 转为 Vale TablePipeFormat.yml (existence)
  结构项: 表格列数一致 → 真 Gap → Python 检查器
  术语项: 用"API"不用"api" → 已有 Terminology.yml (substitution)
  测试项: 正例/反例 → should-pass.md + should-fail.md
```

## 案例 2: 从 xlsx 测试表格提取

```
输入: 测试用例.xlsx
格式: 表头行 + 数据行（输入 | 期望输出 | 通过条件）

提取出的规范片段（示例）:
  测试项: 每行是一个测试用例
    → 转为 pytest 参数化测试
    → 或转为 should-pass.md / should-fail.md 用于 test-rule
  术语项: 某列是术语定义
    → 转为 glossary/terms.yaml
```

## 案例 3: 从 d.ts 类型定义提取

```
输入: api-definitions.d.ts
格式: 接口/类型/枚举声明

提取出的规范片段（示例）:
  术语项: 每个接口名/方法名
    → 转为 glossary/terms.yaml
  结构项: 参数顺序/必需字段
    → 转为 checklist/*.yaml
```
