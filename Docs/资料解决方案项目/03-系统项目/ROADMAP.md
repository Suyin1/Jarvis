# 开发路线图

> 更新时间：2026-06-02

---

## Phase 1: CLI 工具集 (当前)

| 版本 | 任务 | 状态 |
|------|------|------|
| v0.1 | **项目骨架** | ✅ 完成 |
| v0.1 | Vale 集成适配器 (`quality_check`) | ✅ 完成 |
| v0.1 | 检查报告生成器 + Schema | ✅ 完成 |
| v0.1 | `doc-solution check` CLI 工具 | ✅ 完成 |
| v0.1 | MD 解析器 (`md_parser`) | ✅ 完成 |
| v0.1 | Jinja2 模板引擎适配器 | ✅ 完成 |
| v0.1 | `doc-solution generate` CLI 工具 | ✅ 完成 |
| v0.1 | `doc-solution build-kb` CLI 工具 | ✅ 完成 |
| v0.1 | 基础规则集和示例配置 | ✅ 完成 |
| v0.1 | MCP Server 骨架 (Phase 2 预留) | ✅ 完成 |
| v0.2 | MCP Server 完整实现 (JSON-RPC 2.0 + stdio) | ✅ 完成 |
| v0.2 | tools 重构暴露程序化 run_* API | ✅ 完成 |
| v0.2 | MCP Server 测试 (19 个协议/执行测试) | ✅ 完成 |
| v0.1 | 测试套件 (19 个通过) | ✅ 完成 |
| v0.2 | 知识库引擎 (`engine/knowledge/`) | 📅 待开始 |
| v0.2 | d.ts 解析器 (`dts_parser.py`) | 📅 待开始 |
| v0.2 | `doc-solution resolve` 问题解决命令 | 📅 待开始 |
| v0.2 | `doc-solution scan` 维护扫描命令 | 📅 待开始 |
| v0.2 | Vale 规则集完善 (更多检查项) | 📅 待开始 |

## Phase 2: MCP 封装 (按需)

| 任务 | 依赖 | 状态 |
|------|------|------|
| JSON-RPC 2.0 协议层 | - | ✅ 完成 |
| stdio 传输模式 (Content-Length 帧) | - | ✅ 完成 |
| quality_check 工具注册 | tools/check.py | ✅ 完成 |
| generate_content 工具注册 | tools/generate.py | ✅ 完成 |
| build_knowledge 工具注册 | tools/build_kb.py | ✅ 完成 |
| OpenCode 集成验证 | opencode.json | ✅ 完成 |
| 测试套件 (19 个测试) | - | ✅ 完成 |
| SSE 传输模式 (可选) | 团队协作需求 | 📅 待开始 |

## Phase 3: 全平台 (未来)

| 方向 | 触发条件 | 状态 |
|------|----------|------|
| Web 管理后台 | 团队 > 20 人 | 📅 远期 |
| 多人协作平台 | 多团队/多客户 | 📅 远期 |
| SaaS 服务 | 商业化需求 | 📅 远期 |

---

## 里程碑

| 里程碑 | 版本 | 完成时间 | 说明 |
|--------|------|----------|------|
| MVP 可用 | v0.1 | 2026-06-02 | check + generate + build-kb 三个命令可用，19 测试通过 |
| MCP 集成 | v0.2 | 2026-06-02 | MCP Server (JSON-RPC 2.0 + stdio) + OpenCode 验证 + 19 测试 |
| 核心闭环 | v0.3 | 后续 | 知识库引擎 + d.ts 解析器 + 问题解决/扫描命令 |
| Phase 1 稳定 | v1.0 | 后续 | 全部 CLI 命令稳定，API 不再 breaking change |
