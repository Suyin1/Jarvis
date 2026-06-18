---
audience: customer
priority: high
purpose: 面向客户项目团队和决策者的产品概览
category: guide
last-updated: 2026-06-18
---

# 资料解决方案系统 — 产品指南

> 读者：客户项目团队和决策者
> 目的：了解本系统的功能及其对团队的帮助

---

## 这是什么？

资料解决方案系统是一个**自动化文档开发工具包**。它帮助您的团队：

- **检查**文档质量（结构、格式、术语）
- **生成**基于模板的文档（API 参考、开发指南等）
- **构建**知识库，捕获团队的写作风格和规范

全部功能 **100% 离线**运行在您的机器上。数据永远不会离开您的网络。

## 核心能力

### 质量检查

自动检查文档的以下方面：

| 检查类型 | 发现的问题 | 示例 |
|----------|-----------|------|
| 结构 | 缺少标题、标题级别错误、跳过章节 | "缺少 H1 标题" |
| 格式 | 段落过长、代码块缺少语言标签 | "段落超过 200 个字符" |
| 风格 | 术语不一致、非标准措辞 | "应使用'API'而非'api'" |

### 内容生成

从预置模板创建文档：

```
输入：模板（如 API 参考模板）+ 参数（API 名称、参数等）
流程：Jinja2 渲染 + 自动质量检查
输出：格式化文档，可供审查
```

### 知识库

系统提供结构化的**知识库框架**，用于存储和管理团队的文档知识：

```
您的文档 + 规则 + 模板 + 术语
      |
      v
构建知识库 --> 集中注册表（config.yaml）
           --> 规则存储（Vale YAML + 自定义规则）
           --> 模板注册表（Jinja2）
           --> 术语表 + 检查清单存储
```

`build-kb` 命令创建目录结构和注册表，而**语义内容**（术语规则、风格规范、术语表）由您的团队或 AI Agent 按照项目的知识构建方法论提供。

> 关于构建知识库内容的完整方法论，请参见 `docs/kb-construction-guide.md`。

## 工作流程场景

### 新项目搭建

```
1. 收集现有文档和模板
2. 运行 "build-kb" 创建知识库
3. 对示例文档运行 "check" 验证
4. 使用 "generate" 开始生成新文档
```

### 日常开发

```
1. 编写或生成文档内容
2. 运行 "check" 进行质量验证
3. 审查并修复发现的问题
4. 交付给相关人员
```

### 质量审计

```
1. 对整个文档集运行 "check"
2. 获取全面的质量报告
3. 识别常见问题和模式
4. 更新规则以防止未来问题
```

## 系统组件

```
+-------------------------------------------+
|           AI Agent（可选）                  |
|  OpenCode、Cline、Claude Code 等           |
+-------------------------------------------+
              |           |
     (MCP/stdin)    (CLI/shell)
              v           v
+-----------+-----------+-------------------+
| MCP Server| CLI 工具  | 知识库            |
| (stdio)   | (终端)    | (规则/模板)       |
+-----------+-----------+-------------------+
              |
              v
+-------------------------------------------+
|           引擎（Python）                   |
|   解析器 / 规则引擎 / 报告生成器          |
+-------------------------------------------+
```

您可以选择：
- **CLI 命令**直接在终端中使用
- **AI Agent 集成**通过 MCP 协议（如果工作流程使用 AI）

## 安全性

- **100% 离线** — 无网络请求，无数据泄露
- **无 LLM 依赖** — 本系统是工具，而非 AI 模型
- **内置二进制** — Vale 检查工具包含在项目中
- **本地配置** — 所有规则和模板均为本地文件

详见 `docs/customer/SECURITY.md` 的完整安全细节。

## 快速开始

```bash
# 检查文档
doc-solution check --target ./my-document.md

# 生成 API 参考
doc-solution generate --template api-ref --params '{"api_name": "myFunction"}'

# 从您的文档构建知识库
doc-solution build-kb --input ./my-docs/ --name "我的团队"
```

## 验证与测试

安装或接收到本系统后，可以通过以下方法验证系统是否正常工作。

### 运行方式

两种方式等效，二选一即可：

```bash
# 方式一：系统命令（需要 pip install -e . 注册）
doc-solution check --target ./文档.md

# 方式二：直接调用 Python 模块（免安装，项目目录下执行）
python -m tools.cli check --target ./文档.md
```

### 快速验证：内置检查（无需 Vale）

以下检查由系统 Python 代码内置实现，不依赖任何外部工具，开箱即用：

```bash
# 结构检查：标题层级、必需章节
python -m tools.cli check --target 文档.md --check-type structure --output text

# 格式检查：段落长度、代码块语言标注
python -m tools.cli check --target 文档.md --check-type format --output text

# JSON 输出（便于程序解析）
python -m tools.cli check --target 文档.md --check-type structure --output json
```

验证后，检查报告会显示检查总数、违规项清单及综合评分。

### 验证 Vale 规则

风格检查（术语、规范等）依赖 Vale 工具。项目已内置 `knowledge/vale.exe`（v3.15.1），**无需额外安装**，开箱即用。

运行检查：

```bash
doc-solution check --target 文档.md
# 或免安装模式
python -m tools.cli check --target 文档.md
```

### 测试单条规则

单独验证某条 Vale 规则是否生效：

```bash
python -m tools.cli test-rule --rule 规则.yml --should-fail 应触发.md --should-pass 不应触发.md
```

测试结果会显示规则语法是否有效、正向/负向测试是否通过。

### 运行开发测试

运行系统自带的全部单元测试（39 个）：

```bash
python -m pytest tests/ -v
```

所有测试通过表示系统核心功能正常。

## 获取帮助

| 资源 | 涵盖内容 |
|------|----------|
| `USAGE.md` | 完整命令参考 |
| `docs/customer/SECURITY.md` | 安全和隐私细节 |
| `docs/cli-tools.md` | 所有 CLI 选项和示例 |
| `docs/knowledge-base.md` | 如何构建和维护知识库 |
