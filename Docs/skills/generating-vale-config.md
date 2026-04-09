# Skill: generating-vale-config

## 概述

此 Skill 用于根据提供的规范文档（md、xlsx、docx 等格式）自动生成 Vale 规则配置。

## 输入

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| spec_files | array | 是 | 规范文件路径列表 |
| output_dir | string | 是 | Vale 配置输出目录 |
| rule_level | string | 否 | 规则级别 (error/warning/suggestion)，默认 warning |

## 输出

- Vale 配置文件 (`.vale.ini`)
- 规则文件 (YAML 格式)
- 元数据文件 (`meta.json`)

## 功能

### 1. 规范文件解析

支持解析以下格式的规范文档：

| 格式 | 处理方式 |
|------|----------|
| `.xlsx` | 使用 openpyxl 解析 Excel 表格 |
| `.md` | 解析 Markdown 表格和列表 |
| `.docx` | 使用 python-docx 解析 Word 表格 |

### 2. 规则自动生成

根据规范内容自动生成对应的 Vale 规则：

| 规范类型 | 生成规则类型 |
|----------|--------------|
| 禁用词汇 | `existence` |
| 词汇替换 | `substitution` |
| 标点规范 | `substitution` |
| 句子长度 | `match` |
| 段落长度 | `match` |
| 被动语态 | `existence` |

### 3. 配置生成

生成完整的 `.vale.ini` 配置文件，包含：
- `StylesPath`: 规则目录
- `MinAlertLevel`: 最低告警级别
- 文件类型匹配规则

## 使用示例

### 示例 1: 基本使用

```
用户：根据规范文件生成 Vale 配置
输入：
  spec_files: ["docs/规范文档.xlsx", "docs/词汇规范.md"]
  output_dir: "vale-config"
Agent 调用：
  generating-vale-config skill
输出：
  生成 vale-config/.vale.ini
  生成 vale-config/styles/*.yml
```

### 示例 2: 指定规则级别

```
用户：根据规范生成严格配置
输入：
  spec_files: ["rules.xlsx"]
  output_dir: "vale"
  rule_level: "error"
Agent 调用：
  generating-vale-config skill
输出：
  所有规则级别设为 error
```

## 实现逻辑

### 流程图

```
┌─────────────────────────────────────┐
│         输入：规范文件列表           │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│       遍历每个规范文件               │
│  ┌─────────────────────────────┐    │
│  │  1. 识别文件格式            │    │
│  │  2. 解析规范内容            │    │
│  │  3. 转换为中间格式          │    │
│  └─────────────────────────────┘    │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│      合并相同类型的规则              │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│    生成 Vale 规则文件 (YAML)         │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│      生成 .vale.ini 配置             │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│         输出：完整配置               │
└─────────────────────────────────────┘
```

### 核心代码

```python
def generate_vale_config(spec_files, output_dir, rule_level='warning'):
    """主函数"""
    all_rules = {}
    
    # 1. 解析所有规范文件
    for spec_file in spec_files:
        rules = parse_spec_file(spec_file)
        merge_rules(all_rules, rules)
    
    # 2. 生成规则文件
    for rule_name, rule_config in all_rules.items():
        rule_config['level'] = rule_level
        write_yaml(f"{output_dir}/styles/{rule_name}.yml", rule_config)
    
    # 3. 生成 .vale.ini
    generate_vale_ini(output_dir, all_rules.keys())
```

## 规范文件格式要求

### Excel 格式

| 列名 | 说明 | 示例 |
|------|------|------|
| 规则名称 | 规则唯一标识 | banned_words |
| 类型 | Vale 规则类型 | existence/substitution/match |
| 匹配 | 要匹配的文本或正则 | 利用 |
| 替换 | 替换为目标文本 | 使用 (仅 substitution) |
| 消息 | 告警消息 | 建议使用规范词汇 |

### Markdown 格式

```markdown
# 禁用词汇表

| 禁用词 | 推荐词 |
|--------|--------|
| 利用 | 使用 |
| 截止 | 截至 |

# 被动语态

- is
- are
- was
- were
```

## 验证配置

生成配置后，可使用以下命令验证：

```bash
# 检查配置语法
vale ls-config

# 测试规则
vale --config=.vale.ini --output=JSON test.md
```

## 错误处理

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| 文件格式不支持 | 不支持的扩展名 | 使用 xlsx/md/docx 格式 |
| 表格解析失败 | 表格结构不符合规范 | 检查表格表头 |
| YAML 格式错误 | 中文编码问题 | 使用 UTF-8 编码保存 |

## 依赖

- Python 3.8+
- openpyxl (Excel 处理)
- python-docx (Word 处理)
- pyyaml (YAML 生成)

## 扩展

### 添加新格式支持

在 `parse_spec_file` 函数中添加新的解析器：

```python
def parse_spec_file(file_path):
    ext = Path(file_path).suffix.lower()
    
    if ext == '.xlsx':
        return parse_excel(file_path)
    elif ext == '.md':
        return parse_markdown(file_path)
    elif ext == '.docx':
        return parse_word(file_path)
    elif ext == '.json':
        return parse_json(file_path)  # 新增
    else:
        raise ValueError(f"不支持的格式: {ext}")
```

## 相关 Skill

- `checking-docs-guide-quality`: 使用生成的配置检查文档
- `checking-sample-code-quality`: 代码质量检查
- `generating-doc-section`: 文档章节生成
