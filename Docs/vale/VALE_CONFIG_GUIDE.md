# Vale 配置生成指南

## 概述

本文档说明如何将各种格式的规范文档（md、xlsx、docx等）转换为 Vale 规则配置。

---

## 一、Vale 配置基础

### 1.1 配置结构

```
.vale.ini              # 主配置文件
├── StylesPath/       # 规则目录
│   ├── Microsoft/    # 内置样式
│   ├── Readability/  # 内置样式
│   └── custom/       # 自定义规则
│       ├── Rule1.yml
│       ├── Rule2.yml
│       └── meta.json # 样式包元数据
└── Vocab/            # 词汇表
    ├── accept.txt    # 认可的词汇
    └── reject.txt    # 拒绝的词汇
```

### 1.2 .vale.ini 配置项

```ini
# 主配置文件
[General]
StylesPath = .github/styles    # 规则目录路径
MinAlertLevel = suggestion     # 最低告警级别 (error/warning/suggestion)
Vocab = MyProject              # 词汇表名称

# 文件类型匹配
[*.md]                         # Markdown 文件
BasedOnStyles = Microsoft, Vale

[*.txt]
BasedOnStyles = Vale
```

---

## 二、规范到 Vale 规则转换

### 2.1 常见规范类型映射

| 规范类型 | Vale 规则类型 | 说明 |
|----------|---------------|------|
| 禁用词汇 | `existence` | 检查是否包含禁用词 |
| 强制词汇 | `existence` | 检查是否包含必需词 |
| 标点规范 | `substitution` | 替换不规范的标点 |
| 句子长度 | `match` | 检查句子长度 |
| 段落长度 | `match` | 检查段落长度 |
| 大小写规范 | `capitalization` | 检查标题大小写 |
| 被动语态 | `existence` | 检查被动语态 |

### 2.2 规则模板

#### 2.2.1 禁用词汇规则

```yaml
# 文件: styles/custom/BannedWords.yml
extends: existence
message: "禁止使用 '%s'，请使用 '%s'"
ignorecase: true
level: error
swap:
  利用: 使用
  截止: 截至
  逾期: 到期
```

#### 2.2.2 被动语态规则

```yaml
# 文件: styles/custom/PassiveVoice.yml
extends: existence
message: "避免使用被动语态"
ignorecase: true
level: warning
tokens:
  - is
  - are
  - was
  - were
  - been
  - being
```

#### 2.2.3 句子长度规则

```yaml
# 文件: styles/custom/SentenceLength.yml
extends: match
message: "句子过长 (%d 个字符)，建议拆分为多个短句"
level: warning
max: 100
regex: ^.{1,}$
```

#### 2.2.4 标点符号规则

```yaml
# 文件: styles/custom/Punctuation.yml
extends: substitution
message: "使用中文标点 '%s' 替代 '%s'"
ignorecase: true
level: warning
swap:
  ,: ，
  .: 。
  ?: ？
  !: ！
  (: （
  ): ）
```

---

## 三、从 Excel 转换

### 3.1 规范 Excel 结构

假设有一个规范 Excel 文件包含以下列：

| A | B | C | D |
|---|---|---|---|
| 规则名称 | 检查类型 | 匹配模式 | 替换为 |
| 禁用词 | existence | 利用 | 使用 |
| 禁用词 | existence | 截止 | 截至 |

### 3.2 转换脚本逻辑

```python
# excel_to_vale.py
import pandas as pd
import yaml

def excel_to_vale_rules(excel_path, output_dir):
    df = pd.read_excel(excel_path)
    
    rules = {}
    for _, row in df.iterrows():
        rule_name = row['规则名称']
        rule_type = row['检查类型']
        pattern = row['匹配模式']
        replacement = row.get('替换为', '')
        
        if rule_name not in rules:
            rules[rule_name] = {
                'extends': rule_type,
                'message': f"发现规则 '{rule_name}'",
                'level': 'warning',
                'swap' if rule_type == 'substitution' else 'tokens': {}
            }
        
        if rule_type == 'substitution':
            rules[rule_name]['swap'][pattern] = replacement
        else:
            rules[rule_name]['tokens'].append(pattern)
    
    # 输出 YAML 文件
    for name, config in rules.items():
        with open(f"{output_dir}/{name}.yml", 'w', encoding='utf-8') as f:
            yaml.dump(config, f, allow_unicode=True)
```

---

## 四、从 Markdown 转换

### 4.1 规范 MD 结构

```markdown
# 禁用词汇

## 词汇替换
| 禁用词 | 推荐词 |
|--------|--------|
| 利用 | 使用 |
| 截止 | 截至 |

## 被动语态
- is
- are
- was
- were
```

### 4.2 转换脚本逻辑

```python
# md_to_vale.py
import re
import yaml

def md_to_vale_rules(md_path, output_dir):
    with open(md_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 解析表格
    table_pattern = r'\| (.+) \| (.+) \|'
    matches = re.findall(table_pattern, content)
    
    swap_dict = {}
    for old, new in matches:
        swap_dict[old.strip()] = new.strip()
    
    if swap_dict:
        rule = {
            'extends': 'substitution',
            'message': "建议使用规范词汇",
            'level': 'warning',
            'swap': swap_dict
        }
        with open(f"{output_dir}/Vocabulary.yml", 'w', encoding='utf-8') as f:
            yaml.dump(rule, f, allow_unicode=True)
```

---

## 五、从 Word 转换

### 5.1 处理流程

1. 使用 `python-docx` 读取 Word 文档
2. 解析表格内容
3. 转换为 Vale 规则

### 5.2 转换脚本

```python
# docx_to_vale.py
from docx import Document
import yaml

def docx_to_vale_rules(docx_path, output_dir):
    doc = Document(docx_path)
    
    rules = {}
    for table in doc.tables:
        headers = [cell.text for cell in table.rows[0].cells]
        
        for row in table.rows[1:]:
            cells = [cell.text for cell in row.cells]
            data = dict(zip(headers, cells))
            
            rule_name = data.get('规则名称', 'custom')
            if rule_name not in rules:
                rules[rule_name] = {
                    'extends': data.get('类型', 'existence'),
                    'message': data.get('说明', ''),
                    'level': data.get('级别', 'warning'),
                    'swap' if data.get('类型') == 'substitution' else 'tokens': {}
                }
            
            if data.get('类型') == 'substitution':
                rules[rule_name]['swap'][data['匹配']] = data.get('替换', '')
            else:
                rules[rule_name]['tokens'].append(data['匹配'])
    
    for name, config in rules.items():
        with open(f"{output_dir}/{name}.yml", 'w', encoding='utf-8') as f:
            yaml.dump(config, f, allow_unicode=True)
```

---

## 六、批量生成工具

### 6.1 统一转换脚本

```python
#!/usr/bin/env python3
# convert_rules.py
"""
规范文档转 Vale 配置工具
支持: xlsx, md, docx
"""

import sys
import os
from pathlib import Path

def convert_file(input_file, output_dir):
    ext = Path(input_file).suffix.lower()
    
    if ext == '.xlsx':
        from excel_to_vale import excel_to_vale_rules
        excel_to_vale_rules(input_file, output_dir)
    elif ext == '.md':
        from md_to_vale import md_to_vale_rules
        md_to_vale_rules(input_file, output_dir)
    elif ext == '.docx':
        from docx_to_vale import docx_to_vale_rules
        docx_to_vale_rules(input_file, output_dir)
    else:
        print(f"不支持的文件类型: {ext}")
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("用法: python convert_rules.py <输入文件> <输出目录>")
        sys.exit(1)
    
    convert_file(sys.argv[1], sys.argv[2])
```

### 6.2 使用方法

```bash
# 转换 Excel 规范
python convert_rules.py rules.xlsx output/styles

# 转换 Markdown 规范
python convert_rules.py rules.md output/styles

# 转换 Word 规范
python convert_rules.py rules.docx output/styles
```

---

## 七、自定义样式包

### 7.1 样式包结构

```
custom-style/
├── meta.json              # 样式包元数据
├── Passive.yml           # 被动语态规则
├── Vocabulary.yml        # 词汇规则
├── Punctuation.yml      # 标点规则
└── README.md             # 说明文档
```

### 7.2 meta.json 示例

```json
{
  "name": "custom-style",
  "description": "自定义文档规范样式",
  "version": "1.0.0",
  "author": "Your Team",
  "license": "MIT",
  "rules": [
    "Passive.yml",
    "Vocabulary.yml",
    "Punctuation.yml"
  ]
}
```

---

## 八、最佳实践

### 8.1 规则组织建议

| 场景 | 推荐组织方式 |
|------|--------------|
| 词汇规范 | 按主题分组 (如: Vocabulary.yml) |
| 格式规范 | 按文件类型分组 (如: Markdown.yml) |
| 业务规则 | 按项目分组 (如: ProjectA.yml) |

### 8.2 规则优先级

1. **error**: 语法错误、必填项缺失
2. **warning**: 格式问题、建议改进
3. **suggestion**: 风格建议、可选优化

### 8.3 调试技巧

```bash
# 查看所有规则
vale ls-config

# 检查特定文件
vale --config=.vale.ini --output=JSON file.md

# 调试规则
vale compile your-rule.yml
```

---

## 九、常见问题

### Q1: 规则不生效？

检查:
1. `.vale.ini` 中 `StylesPath` 是否正确
2. 规则文件是否在 `StylesPath` 目录下
3. 规则文件名是否与 `BasedOnStyles` 匹配

### Q2: 中文规则报错？

确保 YAML 文件使用 UTF-8 编码，且无 Tab 缩进

### Q3: 如何测试规则？

```bash
# 使用 --dry 模式测试
vale --config=.vale.ini --dry-run file.md
```

---

## 附录

### A. Vale 规则类型参考

| 类型 | 说明 | 适用场景 |
|------|------|----------|
| existence | 检查是否出现 | 禁用词、被动语态 |
| substitution | 替换内容 | 标点、中英文转换 |
| match | 正则匹配 | 句子长度、格式 |
| capitalization | 大小写 | 标题大小写 |
| repetition | 重复检测 | 连续单词 |
| conditional | 条件判断 | 复杂逻辑 |

### B. 内置样式参考

- **Vale**: 基础规则
- **Microsoft**: Microsoft 写作风格
- **Google**: Google 开发者文档风格
- **Alex**: 检测不包容性语言
- **Readability**: 可读性评分
- **WriteGood**: 常见写作问题

### C. 相关资源

- [Vale 官方文档](https://vale.sh/docs/)
- [Vale 规则示例](https://github.com/errata-ai/vale/tree/master/styles)
- [YAML 语法参考](https://yaml.org/)
