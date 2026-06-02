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

## Documentation

### System Architecture

| Document | Description |
|----------|-------------|
| `docs/architecture.md` | Full system architecture, layers, data flow, design principles |
| `docs/knowledge-base.md` | Knowledge base construction, configuration, maintenance |
| `docs/vale-checking.md` | Vale integration, rule system, checking workflow |
| `docs/cli-tools.md` | CLI tool reference with all options and examples |
| `docs/mcp-server.md` | MCP Server setup, tools, protocol details |

### User Guides

| Document | Audience |
|----------|----------|
| `PRODUCT_GUIDE.md` | Customer team: how to use the system |
| `USAGE.md` | AI Agent: how to call system capabilities |
| `TESTING_GUIDE.md` | Developer/AI: regression testing workflow |

### Project Documents

| Document | Purpose |
|----------|---------|
| `AGENTS.md` | AI Agent maintenance guide |
| `ROADMAP.md` | Development roadmap |
| `DEVELOPMENT_LOG.md` | Change history |
| `02-设计方案/演进策略与架构决策记录.md` | Evolution strategy and ADRs |

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
- **Vale optional** (graceful degradation)
- **Windows GBK** encoding safe

## Current Status

- Phase 1 (CLI): Complete - 3 working commands
- Phase 2 (MCP): Complete - 3 tools via JSON-RPC 2.0
- 38 tests passing
- Version 0.2.0
