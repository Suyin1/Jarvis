# 资料解决方案系统 — 回归测试指南

> 面向 AI Agent 的全量回归测试流程。每次修改代码后，请按本文档执行测试。

---

## 一、测试原则

| 原则 | 说明 |
|------|------|
| 每次修改后必须测试 | 确保存量功能不受影响 |
| 新功能必须新增测试 | 确保新内容可验收 |
| 所有测试必须通过 | 不允许跳过失败测试 |
| 失败必须修复或记录 | 记录到 DEVELOPMENT_LOG.md |

## 二、全量回归测试流程

### 步骤 1：运行单元测试

```bash
cd 03-系统项目
python -m pytest tests/ -v
```

**期望结果：** 所有测试通过，无 FAILED 或 ERROR。

**预期输出：**

```
tests/test_md_parser.py::test_parse_headings PASSED
tests/test_md_parser.py::test_parse_code_blocks PASSED
...
tests/test_vale_adapter.py::test_file_result_counts PASSED

======================= 19 passed in 0.05s ========================
```

### 步骤 2：CLI 功能验证

#### 2.1 check 命令 — 结构检查

```bash
python -m tools.cli check \
  --target examples/sample-docs/sample-guide.md \
  --check-type structure \
  --output json
```

**验证点：** 输出为合法 JSON，包含 metadata / summary / details / trace 四个字段。

#### 2.2 check 命令 — 不存在文件

```bash
python -m tools.cli check --target /nonexistent/path
```

**验证点：** 报错退出，提示"目标路径不存在"。

#### 2.3 check 命令 — 目录检查

```bash
python -m tools.cli check \
  --target examples/sample-docs/ \
  --check-type structure
```

**验证点：** 能正确扫描目录下所有 .md 文件。

#### 2.4 check 命令 — 保存报告

```bash
python -m tools.cli check \
  --target examples/sample-docs/sample-guide.md \
  --output json \
  --save-report /tmp/test-report.json
```

**验证点：** 报告文件已生成，内容为合法 JSON。

#### 2.5 generate 命令 — API 参考

```bash
python -m tools.cli generate \
  --template api-ref \
  --params '{"api_name": "testAPI", "declaration": "function test(): void;"}' \
  --output /tmp/test-api.md
```

**验证点：** 文件已生成，包含 api_name，自动执行了质量检查。

#### 2.6 generate 命令 — 不存在模板

```bash
python -m tools.cli generate --template nonexistent
```

**验证点：** 报错提示模板未找到。

#### 2.7 build-kb 命令 — 构建知识库

```bash
python -m tools.cli build-kb \
  --input examples/sample-docs/ \
  --name "测试客户" \
  --output /tmp/test-kb
```

**验证点：** 输出目录包含 config.yaml / rules / templates 等。

#### 2.8 build-kb 命令 — 覆盖已有

```bash
python -m tools.cli build-kb \
  --input examples/sample-docs/ \
  --name "测试客户" \
  --output /tmp/test-kb \
  --force
```

**验证点：** 能覆盖已有目录，不报错。

### 步骤 3：Vale 可选功能验证

```bash
# 如果安装了 Vale
python -m tools.cli check --target examples/sample-docs/ --check-type format
```

**验证点：** Vale 检查可正常执行（如有 Vale）。

### 步骤 4：JSON Schema 校验

生成的检查报告应满足 `schemas/check-report.schema.json` 定义。

## 三、新增功能时的测试规范

### 3.1 新增引擎模块

```python
# tests/test_xxx.py
def test_basic():
    assert True  # 至少有基本功能测试

def test_edge_cases():
    pass  # 边界条件测试

def test_error_handling():
    pass  # 异常处理测试
```

### 3.2 新增 CLI 命令

- 新增命令必须在 `tests/` 中添加对应的测试
- 至少覆盖：正常路径、异常路径、边界条件
- CLI 测试可以通过 `python -m tools.cli <command>` 运行验证

### 3.3 测试文件命名规范

```
tests/test_<模块名>.py
```

示例：
- `engine/parser/md_parser.py` → `tests/test_md_parser.py`
- `engine/checker/reporter.py` → `tests/test_reporter.py`
- `engine/rule_engine/vale_adapter.py` → `tests/test_vale_adapter.py`

## 四、测试通过标准

| 级别 | 要求 |
|------|------|
| P0 | `pytest tests/ -v` 全部通过 |
| P1 | CLI 功能验证 (步骤 2) 全部正常 |
| P2 | 新功能对应的测试已添加并通过 |
| P3 | 不影响已有测试结果 |

## 五、自动化回归 (可选)

在 CI/CD 中可配置：

```bash
# CI 脚本
python -m pytest tests/ -v --tb=short
python -m tools.cli check --target examples/sample-docs/ --check-type structure
python -m tools.cli generate --template api-ref --params '{"api_name": "test"}' --output /tmp/_ci_test.md
rm -f /tmp/_ci_test.md /tmp/test-report.json /tmp/test-kb -rf
```
