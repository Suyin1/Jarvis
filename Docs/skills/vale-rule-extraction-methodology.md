---
audience: ai-agent
category: guide
priority: high
purpose: 从客户文档规范中提取 Vale 规则的可复用方法论
last-updated: 2026-06-07
---

# 规则提取方法论

> 从客户提供的样式规范文档中，系统化提取 Vale 可检测规则的工作流。

---

## 核心原则

**一条规则 = 一个可被程序检测的违规模式。**

不是每个规范章节产出一条规则。一个章节可能产出:
- 多条 Vale 规则（不同的违规模式）
- 几个真 Gap（需要自定义检查器）
- 0 条规则（纯风格指南无法自动检测）

---

## 五步提取法

### Step 1: 分解规范章节

每个章节包含三要素：

```
标准格式  →  "什么是正确写法"
违规模式  →  "什么是错误写法"（程序要检测的目标）
报错信息  →  "报什么错"
```

**操作方法：** 逐句阅读，把每个"问题现象"中的错误模式摘出来。

### Step 2: 选择 Vale 类型

对照决策树：

```
这个违规模式是一个...
  需要禁止的词/模式       → existence
  需要替换的错词          → substitution
  某模式出现次数超限      → occurrence  
  重复词                 → repetition
  前后写法不统一          → consistency
  条件依赖的检查          → conditional
  大小写规范             → capitalization
  拼写检查               → spelling
  数值指标（长度/数量）   → metric
```

### Step 3: 选择 scope

对照表：

| 违规发生在... | scope |
|-------------|-------|
| Markdown 标记本身（\| \> \*\*） | raw |
| 纯文本内容（去掉标记后） | text |
| 标题行内 | heading |
| 代码块内 | code |
| 表格行内 | table |
| 列表项内 | list |
| 块引用内 | blockquote |

### Step 4: 写正则（留意 RE2 限制）

Vale 使用 Go 的 regexp 引擎（RE2），不支持：
- ❌ 零宽断言（lookahead/lookbehind）
- ❌ 反向引用（backreference）
- ❌ 递归匹配

支持：
- ✅ 基本字符类 `[a-z]` `[^abc]`
- ✅ 锚点 `^` `$` `\b`
- ✅ 量词 `*` `+` `?` `{n,m}`
- ✅ 转义 `\s` `\d` `\*` `\(`
- ✅ 非贪婪 `*?` `+?`

### Step 5: 测试验证

每条规则需两个测试文件：

```
rule-name/
  ├── rule-name.yml       # Vale 规则
  ├── should-fail.md      # 故意违规 → 应触发告警
  └── should-pass.md      # 完全合规 → 应无告警
```

验证命令：

```bash
doc-solution test-rule \
  --rule rule-name.yml \
  --should-fail should-fail.md \
  --should-pass should-pass.md
```

---

## 真 Gap 的判断标准

如果 5 步走不通，标记为 Gap。典型的真 Gap：

| 模式 | 原因 | 替代方案 |
|------|------|---------|
| "列数不一致" | 需要解析表格结构，比较每行 | Python 检查器 |
| "缩进层级错误" | 需要文档树上下文 | Python 检查器 + AST |
| "开闭 fence 区分" | 相同文本模式 | occurrence 计数法（有局限） |
| "跨行对齐" | 需要多行比较 | Python 检查器 |
| "跨文件检查" | 单文件工具限制 | 脚本扫描 |

---

## 案例: 从 md-style-check.md 提取

### 章节: 表格格式

```
Step 1 分解:
  - 违规1: 表格行中有制表符\t
  - 违规2: 行首/尾无管道符|
  - 违规3: 分隔符含空格/错误符号
  - 违规4: 列数不一致

Step 2 选类型:
  - 违规1 → existence（禁止制表符）
  - 违规2 → existence（检测 ┤ 或 ├ 模式）
  - 违规3 → existence（检测非-分隔符）
  - 违规4 → ❌ 真Gap（需比较每行列数）

Step 3 选 scope:
  - 全部用 raw（因为要检查 | - \t 等标记字符）

Step 4 写正则:
  - 违规1: \t
  - 违规2: ^\|[^|\n]*\|[^|\n]*$（行首|但行尾不是|）
  - 违规3: \| *-+\s+-+ *\|（分隔符含空格）

Step 5 测试:
  - 每条规则创建 should-fail.md + should-pass.md
  - 运行 test-rule 验证 PASS
```

---

## 批量验证方法

所有规则就绪后，应运行 4 阶段验证：

| 阶段 | 验证什么 | 命令 |
|------|---------|------|
| 1. 单元测试 | 每条规则的语法+正/反向测试 | `test-rule` 逐条执行 |
| 2. 集成测试 | 所有规则同时工作 | Vale 检查合成文档 |
| 3. 真实文档 | 在真实文档上的效果 | `check` + `--config` |
| 4. 回归 | 不破坏已有功能 | `pytest tests/` |
