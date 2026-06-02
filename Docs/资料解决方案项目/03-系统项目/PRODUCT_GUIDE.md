---
audience: customer
priority: high
purpose: Product overview for customer project teams and decision-makers
category: guide
last-updated: 2026-06-03
---

# Doc Solution System - Product Guide

> Audience: Customer project teams and decision-makers
> Purpose: Understand what this system does and how it helps your team

---

## What is This?

The Doc Solution System is a **toolkit for automated document development**. It helps your team:

- **Check** document quality automatically (structure, formatting, terminology)
- **Generate** documents from templates (API reference, development guides, etc.)
- **Build** a knowledge base that captures your team's writing style and conventions

All of this runs **100% offline** on your own machines. No data ever leaves your network.

## Core Capabilities

### Quality Checking

Automatically check your documents for:

| Check Type | What It Finds | Example |
|-----------|---------------|---------|
| Structure | Missing titles, wrong heading levels, skipped sections | "H1 heading missing" |
| Format | Overly long paragraphs, code blocks without language labels | "Paragraph exceeds 200 characters" |
| Style | Inconsistent terminology, non-standard phrasing | "Use 'API' instead of 'api'" |

### Content Generation

Create documents from pre-built templates:

```
Input:  Template (e.g., API Reference template) + Parameters (API name, parameters, etc.)
Process: Jinja2 rendering + automatic quality check
Output: Formatted document ready for review
```

### Knowledge Base

The system provides a structured **knowledge base framework** to store and manage your team's documentation knowledge:

```
Your documents + rules + templates + glossary
      |
      v
Build Knowledge Base --> Centralized registry (config.yaml)
                     --> Rule storage (Vale YAML + custom rules)
                     --> Template registry (Jinja2)
                     --> Glossary + checklist storage
```

The `build-kb` command creates the directory structure and registry, while the **semantic content** (terminology rules, style conventions, glossary) is provided by your team or AI Agent following the project's knowledge construction methodology.

> See `docs/kb-construction-guide.md` for the complete methodology on building knowledge base content.

## Workflow Scenarios

### New Project Setup

```
1. Collect your existing documents and templates
2. Run "build-kb" to create a knowledge base
3. Verify with "check" on a sample document
4. Start generating new documents with "generate"
```

### Daily Development

```
1. Write or generate document content
2. Run "check" for quality validation
3. Review and fix any issues found
4. Deliver to stakeholders
```

### Quality Audit

```
1. Run "check" on your entire documentation set
2. Get a comprehensive quality report
3. Identify common issues and patterns
4. Update rules to prevent future issues
```

## System Components

```
+-------------------------------------------+
|           AI Agent (optional)              |
|  OpenCode, Cline, Claude Code, etc.        |
+-------------------------------------------+
              |           |
     (MCP/stdin)    (CLI/shell)
              v           v
+-----------+-----------+-------------------+
| MCP Server| CLI Tools | Knowledge Base    |
| (stdio)   | (terminal)| (rules/templates) |
+-----------+-----------+-------------------+
              |
              v
+-------------------------------------------+
|           Engine (Python)                 |
|   Parser / Rules Engine / Reporter        |
+-------------------------------------------+
```

You can use either:
- **CLI commands** directly in your terminal
- **AI Agent integration** via MCP protocol (if your workflow uses AI)

## Security

- **100% offline** - No network requests, no data leakage
- **No LLM dependency** - The system is a tool, not an AI model
- **Bundled binary** - Vale linter included in the project
- **Local configuration** - All rules and templates are local files

See `docs/customer/SECURITY.md` for complete security details.

## Quick Start

```bash
# Check a document
doc-solution check --target ./my-document.md

# Generate API reference
doc-solution generate --template api-ref --params '{"api_name": "myFunction"}'

# Build knowledge base from your docs
doc-solution build-kb --input ./my-docs/ --name "My Team"
```

## Getting Help

| Resource | What It Covers |
|----------|---------------|
| `USAGE.md` | Complete command reference |
| `docs/customer/SECURITY.md` | Security and privacy details |
| `docs/cli-tools.md` | All CLI options and examples |
| `docs/knowledge-base.md` | How to build and maintain knowledge bases |
