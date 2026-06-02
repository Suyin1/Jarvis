---
audience: ai-agent
priority: high
purpose: Development change history log
category: reference
last-updated: 2026-06-03
---

# 开发进度记录

---

## 2026-06-02 - 项目初始化 Phase 1 + v0.1 MVP 完成

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 项目骨架搭建 | ✅ 完成 | 完整目录结构、pyproject.toml、requirements.txt |
| 演进策略文档 | ✅ 完成 | EVOLUTION_STRATEGY.md，CLI->MCP->全平台演进路径 + ADR |
| AI 维护指南 | ✅ 完成 | AGENTS.md，面向AI Agent的开发维护规范 |
| 项目文档体系 | ✅ 完成 | README.md / ROADMAP.md / DEVELOPMENT_LOG.md |
| Vale 适配器 | ✅ 完成 | engine/rule_engine/vale_adapter.py |
| MD 解析器 | ✅ 完成 | engine/parser/md_parser.py，含标题/代码块/链接提取 |
| 检查报告 Schema | ✅ 完成 | schemas/check-report.schema.json + schemas/skill-def.schema.json + schemas/knowledge-config.schema.json |
| 检查报告生成器 | ✅ 完成 | engine/checker/reporter.py，支持 JSON/Text 输出 |
| CLI check 命令 | ✅ 完成 | tools/cli.py + tools/check.py，结构/Vale/格式三合一检查 |
| CLI generate 命令 | ✅ 完成 | tools/generate.py，基于 Jinja2 模板生成文档 |
| CLI build-kb 命令 | ✅ 完成 | tools/build_kb.py，自动分析输入构建知识库 |
| 知识库模板 | ✅ 完成 | API参考 / 开发指南 Jinja2 模板 |
| 示例配置 | ✅ 完成 | examples/ + 示例文档 |
| MCP Server 骨架 | ✅ 完成 | mcp/server.py，Phase 2 预留 |
| 测试套件 | ✅ 完成 | 19 个测试全部通过 |
| Python 3.6 兼容 | ✅ 完成 | 修复 list/dict 泛型语法、subprocess 参数 |

### 项目结构总览

```
03-系统项目/
├── AGENTS.md              # AI Agent 维护指南
├── README.md              # 项目总览
├── ROADMAP.md             # 开发路线图
├── DEVELOPMENT_LOG.md     # 开发记录 (本文件)
├── pyproject.toml         # 项目配置
├── requirements.txt       # 依赖清单
│
├── engine/                # 核心引擎
│   ├── rule_engine/       # Vale 规则引擎适配器
│   ├── parser/            # MD/d.ts 解析器
│   ├── checker/           # 检查器 + 报告生成器
│   ├── template_engine/   # Jinja2 模板引擎适配器
│   └── knowledge/         # 知识库引擎 (待实现)
│
├── tools/                 # CLI 工具
│   ├── cli.py             # 主入口
│   ├── check.py           # quality-check 命令
│   ├── generate.py        # content-generate 命令
│   └── build_kb.py        # knowledge-build 命令
│
├── mcp/                   # MCP Server (Phase 2 预留)
├── schemas/               # JSON Schema 定义
├── knowledge/             # 默认知识库
├── tests/                 # 测试 (19 个通过)
└── examples/              # 示例配置
```

### 下一步

- 完善测试覆盖（更多边界场景）
- 实现 knowledge 知识库引擎（核心差异化能力）
- 更多 Vale 规则示例
- 文档 d.ts 解析器实现


## 2026-06-02 - v0.1 完善: 文档体系 + 编码兼容 + 全量测试

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 依赖精简 | ✅ 完成 | 移除未使用的 markdown-it-py / jsonschema，确认全部三方库已预装 |
| AI Agent 使用指南 | ✅ 完成 | USAGE.md，含完整命令说明和最佳实践 |
| 客户使用说明 | ✅ 完成 | PRODUCT_GUIDE.md，面向客户的说明书 |
| 回归测试指南 | ✅ 完成 | TESTING_GUIDE.md，AI Agent 全量回归测试流程 |
| GBK 编码兼容 | ✅ 完成 | 移除所有 emoji/非ASCII字符，click.echo 使用 % 格式化 |
| JSON 输出干净 | ✅ 完成 | JSON 模式下不输出进度信息，管道可正常解析 |
| 模板名查找修复 | ✅ 完成 | generate 支持目录名模板 (--template api-ref) |
| 全量测试验证 | ✅ 完成 | 19 单元测试 + 5 项 CLI 功能测试全部通过 |

### 变更详情

- 新增 `USAGE.md` — AI Agent 使用指南
- 新增 `PRODUCT_GUIDE.md` — 客户使用说明
- 新增 `TESTING_GUIDE.md` — 回归测试指南
- 修改 `requirements.txt` — 精简为实际使用的三个依赖
- 修改 `pyproject.toml` — 同步精简依赖声明
- 修改 `engine/checker/reporter.py` — 移除 emoji，使用 ASCII 兼容字符
- 修改 `tools/check.py` — JSON 模式隐藏进度输出
- 修改 `tools/generate.py` — 修复模板名查找，支持目录名模板；替换所有 f-string 为 % 格式化
- 修改 `tools/build_kb.py` — 替换 f-string 为 % 格式化

### 当前代码度量

- Python 文件: 20 个
- 测试: 19 个 (全部通过)
- 三方依赖: 3 个 (click, pyyaml, jinja2, 均已预装)
- 文档: 8 个 (README, AGENTS, USAGE, PRODUCT_GUIDE, TESTING_GUIDE, ROADMAP, DEVELOPMENT_LOG, EVOLUTION_STRATEGY)


## 2026-06-02 - Phase 2 MCP Server 实现 + OpenCode 集成

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| JSON-RPC 2.0 协议层 | 完成 | mcp/protocol.py，含 Content-Length 帧、错误码、消息构造 |
| Stdio 传输 | 完成 | 零网络依赖，stdin/stdout 通信，stderr 日志 |
| tools 重构 | 完成 | check/generate/build_kb 暴露 run_* 程序化 API，CLI 和 MCP 共用 |
| MCP Server | 完成 | mcp/server.py，3 个工具 (quality_check/generate_content/build_knowledge) |
| OpenCode 配置 | 完成 | opencode.json 注册 doc-solution-mcp local server |
| 测试套件 | 完成 | 19 个新测试 (协议层 + 传输 + Server Handshake + 工具执行) |
| pyproject.toml | 完成 | 添加 doc-solution-mcp entry point |

### 变更详情

- 新增 `mcp/protocol.py` — JSON-RPC 2.0 协议层 (StdioTransport, create_response/error/notification, MCPError)
- 重写 `mcp/server.py` — 完整 MCP Server (initialize/tools/list/tools/call 处理)
- 重构 `tools/check.py` — 提取 run_check() 返回 CheckReport，CLI 命令调用它
- 重构 `tools/generate.py` — 提取 run_generate() 返回 (content, report)
- 重构 `tools/build_kb.py` — 提取 run_build_kb() 返回 info dict
- 新增 `tests/test_mcp_server.py` — 19 个测试 (4 个测试类)
- 修改 `pyproject.toml` — 添加 doc-solution-mcp entry point
- 修改 `opencode.json` — 注册 doc-solution-mcp local MCP server
- 修改 `ROADMAP.md` — Phase 2 标记完成，添加 v0.2 里程碑

### MCP Server 使用方式

```bash
# 方式 1: 直接运行 (stdio 模式，供 AI Agent 调用)
python -m mcp.server

# 方式 2: 通过 OpenCode (已配置 opencode.json)
# 重启 OpenCode 后自动发现 doc-solution-mcp 工具

# 方式 3: 通过 entry point (安装后)
doc-solution-mcp
```

### 当前代码度量

- Python 文件: 22 个 (+2: mcp/protocol.py, tests/test_mcp_server.py)
- 测试: 38 个 (全部通过，+19 MCP Server 测试)
- 三方依赖: 3 个 (无新增依赖，纯标准库实现 MCP 协议)
- 文档: 8 个


## 2026-06-02 - 技术文档体系构建

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 架构文档 | 完成 | docs/architecture.md，三层架构、数据流、设计原则 |
| 知识库指南 | 完成 | docs/knowledge-base.md，构建/配置/使用/多客户维护 |
| Vale 检查集成文档 | 完成 | docs/vale-checking.md，集成架构/规则/检查类型/排障 |
| CLI 工具参考 | 完成 | docs/cli-tools.md，完整命令参考、选项、示例 |
| MCP Server 指南 | 完成 | docs/mcp-server.md，工具注册表、协议细节、测试方式 |
| README 重写 | 完成 | 完整的入口文档，链接全部子文档 |
| Vale 可用性验证 | 完成 | Vale 已全局安装，对新建文档检查无违规 |
| 系统自检 | 完成 | check/build-kb/generate 均正常，38 测试通过 |

### 变更详情

- 新增 `docs/architecture.md` — 架构概述、三层分解、数据流、设计原则
- 新增 `docs/knowledge-base.md` — KB 构建步骤、目录结构、配置参考、使用方式、多客户维护
- 新增 `docs/vale-checking.md` — Vale 集成架构、适配器/优雅降级、规则系统、检查类型对照、安装排障
- 新增 `docs/cli-tools.md` — CLI 完整参考、每个命令的选项/示例/退出码
- 新增 `docs/mcp-server.md` — MCP 协议/工具/测试/协议细节
- 重写 `README.md` — 完整的项目入口，含快速开始、文档导航、能力说明、项目结构

### 当前代码度量

- Python 文件: 22 个
- 测试: 38 个 (全部通过)
- 文档: 15 个


## 2026-06-03 - 文档体系重构: 双分类 + YAML frontmatter

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 文档双分类体系 | 完成 | 明确分为"客户文档"和"AI Agent/技术文档"两类 |
| YAML frontmatter | 完成 | 所有文档添加 audience/category/priority/last-updated 字段 |
| AGENTS.md 优化 | 完成 | 添加 frontmatter + 结构化 checklist + 文档索引 |
| README.md 重写 | 完成 | 按受众分类的文档索引入口 |
| PRODUCT_GUIDE.md | 完成 | 添加 YAML frontmatter |
| docs/*.md 标注 | 完成 | 6 个技术文档 + 1 个客户文档全部添加 frontmatter |
| ROADMAP.md / DEVELOPMENT_LOG.md | 完成 | 同步添加 frontmatter |

### 变更详情

- 修改 `AGENTS.md` — 添加 YAML frontmatter + 结构化工作流 + 文档索引表
- 修改 `README.md` — 重写为双分类入口文档，"For Customers" / "For AI Agents" / "For All"
- 修改 `PRODUCT_GUIDE.md` — 添加 YAML frontmatter
- 修改 `docs/architecture.md` — 添加 YAML frontmatter
- 修改 `docs/knowledge-base.md` — 添加 YAML frontmatter
- 修改 `docs/cli-tools.md` — 添加 YAML frontmatter
- 修改 `docs/mcp-server.md` — 添加 YAML frontmatter
- 修改 `docs/vale-checking.md` — 添加 YAML frontmatter
- 修改 `docs/customer/SECURITY.md` — 添加 YAML frontmatter
- 修改 `ROADMAP.md` — 添加 YAML frontmatter
- 修改 `DEVELOPMENT_LOG.md` — 添加 YAML frontmatter + 本次变更记录

### 当前代码度量

- Python 文件: 22 个
- 测试: 39 个 (全部通过)
- 文档: 15 个 (全部有 audience 标注)


## 2026-06-03 - 知识库构建方法论文档 + 产品文档修正

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 产品文档修正 | 完成 | PRODUCT_GUIDE.md 如实描述 build-kb 为"注册器"而非"语义学习器" |
| 新建知识库构建方法论 | 完成 | docs/kb-construction-guide.md，完整 5 步流程 |
| 输入格式全覆盖 | 完成 | 指定 md/xlsx/csv/json/yaml/ts/d.ts/py/j2/txt 的处理策略 |
| Vale 9 种规则模板 | 完成 | existence/substitution/occurrence/repetition/consistency/conditional/capitalization/spelling/metric 全部提供 YAML 模板 |
| 测试标准转换方法论 | 完成 | xlsx 测试用例/ d.ts 类型定义 → checklist 转换流程 |
| 术语提取方法论 | 完成 | 4 条启发式规则 + 多格式输入提取策略 |
| 文档引用更新 | 完成 | AGENTS.md(先读列表)/USAGE.md(build-kb说明)/README.md(文档表)/knowledge-base.md(顶部引导) 全部添加引用 |

### 变更详情

- 创建 `docs/kb-construction-guide.md` — 完整知识库构建方法论文档
- 修改 `PRODUCT_GUIDE.md` — 修正 Knowledge Base 章节描述，如实说明 build-kb 能力
- 修改 `AGENTS.md` — 先读列表添加 kb-construction-guide.md
- 修改 `USAGE.md` — build-kb 小节添加方法论引用；维护清单添加 kb-construction-guide.md
- 修改 `README.md` — 文档表添加 kb-construction-guide.md
- 修改 `docs/knowledge-base.md` — 顶部添加指向 kb-construction-guide.md 的引导
- 修改 `ROADMAP.md` — 添加文档体系重构任务

### 当前代码度量

- Python 文件: 22 个
- 测试: 39 个 (全部通过)
- 文档: 16 个 (+1: docs/kb-construction-guide.md)


## 2026-06-03 - 通用规则测试工具 + KB 构建方法论完整

### 本次工作

| 任务 | 状态 | 说明 |
|------|------|------|
| 创建 test-rule 命令 | 完成 | tools/test_rule.py，通用 Vale 规则测试工具，不依赖具体规则内容 |
| 注册 CLI 命令 | 完成 | cli.py + test-rule 子命令 |
| 创建示例测试文档 | 完成 | examples/test-rules/should-fail-terminology.md + should-pass.md |
| 修复 Terminology.yml 规则 Bug | 完成 | conditional 类型误用导致规则永不触发，改为 substitution 类型 |
| 修复 build_kb.py 生成的规则 | 完成 | 同步修复生成器中的相同 Bug |
| 规则测试方法论 | 完成 | kb-construction-guide.md 新增 Step 3.5 章节 |
| 文档引用 | 完成 | USAGE.md 添加 test-rule 命令参考 + 维护清单更新 |

### 变更详情

- 新增 `tools/test_rule.py` — 通用 Vale 规则测试工具（run_test_rule + test_rule_command），支持语法校验/正测试/负测试
- 修改 `tools/cli.py` — 注册 test_rule_command
- 修改 `knowledge/rules/vale/styles/Custom/Terminology.yml` — conditional→substitution 修复
- 修改 `tools/build_kb.py` — 同步修复生成的 Terminology 规则
- 新增 `examples/test-rules/should-fail-terminology.md` — 正测试文档（故意用小写 api/sdk）
- 新增 `examples/test-rules/should-pass-terminology.md` — 负测试文档（全大写）
- 修改 `docs/kb-construction-guide.md` — 新增 Step 3.5 规则测试完整方法论（含原理/用法/示例/可重复套件）
- 修改 `USAGE.md` — 添加 test-rule 命令参考 + 维护清单更新
- 修改 `ROADMAP.md` — 添加 test-rule 任务
- 修改 `DEVELOPMENT_LOG.md` — 追加本次记录

### 关键设计决策

test-rule 的通用性原理：
- 不关心规则检查什么内容（术语/格式/拼写）
- 只验证三点：Vale 接受此 YAML（语法）、正测试文档触发告警（有效）、负测试文档无告警（无误报）
- 通过临时目录 + 动态 .vale.ini 隔离每条规则，互不干扰

### 当前代码度量

- Python 文件: 23 个 (+1: tools/test_rule.py)
- 测试: 39 个 (全部通过)
- 文档: 16 个
