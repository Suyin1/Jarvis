---
audience: both
priority: high
purpose: Entry point and document index for both customers and AI Agents
category: guide
last-updated: 2026-06-03
---

# Doc Solution System

> Document development full-chain solution: quality checking, content generation, knowledge base building

---

## Quick Start

```bash
# Check if everything works
python -m pytest tests/ -v

# Check a document
python -m tools.cli check --target examples/sample-docs/sample-guide.md

# Generate content from a template
python -m tools.cli generate --template api-ref --params '{"api_name": "startAbility"}'

# Build a knowledge base
python -m tools.cli build-kb --input examples/sample-docs/ --name "Example"
```

## Documentation by Audience

### For Customers

| Document | Description |
|----------|-------------|
| `PRODUCT_GUIDE.md` | Product overview: capabilities, workflow, quick start |
| `docs/customer/SECURITY.md` | Security and privacy: zero-network proof, FAQ |

### For AI Agents / Technical Users

| Document | Category | Description |
|----------|----------|-------------|
| `USAGE.md` | Reference | CLI + MCP tool reference, all options and examples |
| `AGENTS.md` | Guide | AI Agent maintenance workflow and conventions |
| `docs/architecture.md` | Architecture | System layers, data flow, design principles |
| `docs/cli-tools.md` | Reference | Complete CLI command reference |
| `docs/mcp-server.md` | Reference | MCP Server setup, tools, protocol details |
| `docs/knowledge-base.md` | Guide | KB construction, configuration, multi-customer maintenance |
| `docs/kb-construction-guide.md` | Guide | KB content construction: Vale rules, terminology, test conversion |
| `docs/vale-checking.md` | Reference | Vale integration, rule system, offline proof |

### For All

| Document | Purpose |
|----------|---------|
| `ROADMAP.md` | Development roadmap and milestones |
| `DEVELOPMENT_LOG.md` | Change history |
| `02-设计方案/演进策略与架构决策记录.md` | Evolution strategy and architecture decisions |

## Core Capabilities

### 1. Quality Checking

```bash
doc-solution check --target <path> [--check-type all|structure|format|style]
```

Three check levels:
- **Structure**: Heading hierarchy, required sections (built-in, no dependencies)
- **Format**: Paragraph length, code block annotations (built-in + optional Vale)
- **Style**: Terminology, conventions (via Vale, optional)

See `docs/vale-checking.md` for details.

### 2. Content Generation

```bash
doc-solution generate --template <name> --params '<json>'
```

Uses Jinja2 templates with auto quality check after generation.
Templates resolve by name, directory, or file path.
See `docs/cli-tools.md` for template reference.

### 3. Knowledge Base Building

```bash
doc-solution build-kb --input <dir> --name <customer>
```

Analyzes customer source materials to build a KB with:
- Document style profile (headings, paragraph stats)
- Vale configuration and style rules
- Registered templates
- Glossary and checklist templates
See `docs/knowledge-base.md` for details.

### 4. AI Agent Integration (MCP)

The system exposes all capabilities as MCP tools for AI Agent use:

```json
// Configure in opencode.json
{
  "mcp": {
    "doc-solution-mcp": {
      "type": "local",
      "command": ["python", "-m", "mcp.server"],
      "enabled": true
    }
  }
}
```

Available tools: `quality_check`, `generate_content`, `build_knowledge`.
See `docs/mcp-server.md` for details.

## Project Structure

```
.
|-- README.md                    # This file
|-- docs/                        # Technical documentation (new)
|   |-- architecture.md
|   |-- knowledge-base.md
|   |-- vale-checking.md
|   |-- cli-tools.md
|   |-- mcp-server.md
|-- AGENTS.md                    # AI Agent maintenance guide
|-- USAGE.md                     # AI Agent usage guide
|-- PRODUCT_GUIDE.md             # Customer usage guide
|-- TESTING_GUIDE.md             # Regression testing guide
|-- ROADMAP.md                   # Development roadmap
|-- DEVELOPMENT_LOG.md           # Change history
|
|-- engine/                      # Core engine (Python)
|   |-- parser/                  #   Markdown parser
|   |-- rule_engine/             #   Vale adapter
|   |-- checker/                 #   Report generator
|   +-- knowledge/               #   KB engine (placeholder)
|-- tools/                       # CLI entry points
|   |-- cli.py                   #   Main entry
|   |-- check.py                 #   Quality check
|   |-- generate.py              #   Content generation
|   +-- build_kb.py              #   KB building
|-- mcp/                         # MCP Server
|   |-- protocol.py              #   JSON-RPC 2.0 protocol
|   +-- server.py                #   Server + tool registration
|-- knowledge/                   # Default knowledge base
|   |-- config.yaml
|   |-- rules/                   # Vale + custom rules
|   |-- templates/               # Jinja2 templates
|   |-- glossary/                # Terminology
|   +-- checklist/               # Quality checklists
|-- schemas/                     # JSON Schema definitions
|-- tests/                       # 38 passing tests
+-- examples/                    # Sample configurations
```

## Technical Details

- **Python 3.6+** compatible (no 3.7+ syntax)
- **Zero network** dependency (all offline)
- **No LLM** dependency (toolkit, not AI)
- **Vale optional** (graceful degradation, bundled binary at `knowledge/vale.exe`)
- **Windows GBK** encoding safe

## Current Status

- Phase 1 (CLI): Complete - 3 working commands
- Phase 2 (MCP): Complete - 3 tools via JSON-RPC 2.0
- 38 tests passing
- Version 0.2.0
