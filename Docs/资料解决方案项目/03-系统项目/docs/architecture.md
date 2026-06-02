# System Architecture

> Technical architecture of the Doc Solution System

---

## Overview

The Doc Solution System is a **document development full-chain solution** designed for AI Agent integration. It consists of three layers:

```
+---------------------------------------------------+
|                   AI Agent                         |
|  (OpenCode, Cline, Claude Code, etc.)              |
+---------------------------------------------------+
         |  stdin/stdout (MCP)  |  CLI (shell)
         v                     v
+-------------------+  +-------------------+
|   MCP Server      |  |   CLI Tools       |
|   (Phase 2)       |  |   (Phase 1)       |
+-------------------+  +-------------------+
         |                     |
         +----------+----------+
                    v
+-----------------------------------+
|           Engine Layer            |
|  +--------+  +-------+  +------+  |
|  | Parser |  | Rules |  | Check|  |
|  | (MD)   |  | (Vale)|  | Report|  |
|  +--------+  +-------+  +------+  |
+-----------------------------------+
                    |
         +----------+----------+
                    v
+-----------------------------------+
|         Knowledge Base            |
|  (rules/ templates/ glossary/     |
|   checklist/ config)              |
+-----------------------------------+
```

## Layer Details

### 1. CLI Layer (`tools/`)

The CLI layer provides command-line interfaces using `click`. It is the **primary delivery format** for Phase 1.

| Command | Module | Purpose |
|---------|--------|---------|
| `doc-solution check` | `tools/check.py` | Quality checking |
| `doc-solution generate` | `tools/generate.py` | Content generation |
| `doc-solution build-kb` | `tools/build_kb.py` | Knowledge base construction |
| `doc-solution-mcp` | `mcp/server.py` | MCP server (stdio) |

Each command exposes a `run_*()` programmatic API (e.g. `run_check()`, `run_generate()`, `run_build_kb()`) that is shared between CLI and MCP Server.

### 2. MCP Layer (`mcp/`)

The MCP layer implements the Model Context Protocol over stdio for AI Agent integration.

| File | Purpose |
|------|---------|
| `mcp/protocol.py` | JSON-RPC 2.0 protocol + stdio transport |
| `mcp/server.py` | Tool registration and dispatch |

**Protocol**: JSON-RPC 2.0 with Content-Length header framing.

**Exposed Tools**:

| Tool Name | Description | Wraps |
|-----------|-------------|-------|
| `quality_check` | Run quality checks | `tools.check.run_check()` |
| `generate_content` | Generate content from templates | `tools.generate.run_generate()` |
| `build_knowledge` | Build knowledge base | `tools.build_kb.run_build_kb()` |

### 3. Engine Layer (`engine/`)

The engine layer contains all core logic. It has no CLI dependencies and can be imported directly.

| Module | Component | Responsibility |
|--------|-----------|----------------|
| `engine/parser/md_parser.py` | MDParser | Markdown parsing, heading/code block/link extraction, hierarchy validation |
| `engine/rule_engine/vale_adapter.py` | ValeAdapter | Vale CLI wrapper, JSON output parsing, graceful degradation |
| `engine/checker/reporter.py` | CheckReport | Unified report format, JSON/text output, scoring |
| `engine/knowledge/` | (placeholder) | Future: intelligent KB engine |
| `engine/template_engine/` | (placeholder) | Future: template engine adapter |

### 4. Knowledge Base (`knowledge/`)

The knowledge base stores customer-specific configuration and assets. See `docs/knowledge-base.md` for details.

## Data Flow

### Quality Check Flow

```
User/Tool -> run_check(target, check_type)
  |
  +-> MDParser.parse()  -- structure check
  |     +-> check_heading_hierarchy()
  |
  +-> ValeAdapter.check()  -- style/format check
  |     +-> vale CLI (JSON output)
  |     +-> graceful fallback if Vale not found
  |
  +-> Built-in checks  -- format check
  |     +-> code block language annotation
  |     +-> paragraph length
  |
  +-> CheckReport  -- merge all results
        +-> to_json() / to_text()
```

### Content Generation Flow

```
User/Tool -> run_generate(template, params)
  |
  +-> Jinja2 Environment + FileSystemLoader
  +-> Template lookup (name/dir/file)
  +-> Render with params
  +-> Optional: auto_check -> MDParser -> CheckReport
  +-> Return (content, report)
```

### Knowledge Base Build Flow

```
User/Tool -> run_build_kb(input_dir, name)
  |
  +-> Scan input files (.md .yaml .json .py .ts etc.)
  +-> Analyze document style (50 files max)
  |     +-> heading stats
  |     +-> paragraph stats
  +-> Generate Vale config (.vale.ini + styles)
  +-> Register templates from input
  +-> Generate config.yaml
  +-> Generate style-profile.yaml
```

## Design Principles

1. **Local-first**: All core functionality runs offline. Zero network dependencies. Vale binary is bundled at `knowledge/vale.exe` for intranet use.
2. **No LLM dependency**: The system provides tools for AI Agents, not LLM features itself.
3. **Graceful degradation**: Vale is optional. Without it, structure/format checks still work.
4. **CLI/MCP duality**: Every `run_*()` function works identically from CLI and MCP.
5. **Python 3.6 compatible**: No 3.7+ syntax features used.
6. **GBK safe**: No emoji or non-ASCII characters in output.
