---
audience: ai-agent
priority: medium
purpose: Guide for building, configuring, and using the knowledge base
category: guide
last-updated: 2026-06-03
---

# Knowledge Base Guide

> How to build, configure, and use the customer-specific knowledge base

---

## Overview

The Knowledge Base (KB) is the **core differentiating feature** of the Doc Solution System. It captures customer-specific knowledge including document style, terminology, templates, rules, and checklists, enabling automated document development that follows customer conventions.

> **For KB content construction methodology** (how to author Vale rules, extract terminology, convert test standards), see `docs/kb-construction-guide.md`. This guide covers the directory structure and registration process only.

## How the KB is Built

### Step 1: Prepare Input Materials

Collect customer source materials into a single directory:

```
customer-inputs/
  |-- docs/                 # Source documents (.md)
  |   |-- api-reference.md
  |   |-- development-guide.md
  |-- templates/            # Jinja2 template files
  |   |-- api-ref/
  |   |   +-- template.md.j2
  |   +-- dev-guide/
  |       +-- template.md.j2
  |-- rules/                # Custom rules (optional)
  |-- glossary/             # Term definitions (optional)
  +-- checklist/            # Quality checklist (optional)
```

### Step 2: Run build-kb

```bash
doc-solution build-kb --input ./customer-inputs/ --name "CustomerName"
```

This command:

1. **Scans** all files in the input directory (.md, .yaml, .json, .py, .ts, .d.ts, .txt, .j2)
2. **Analyzes** document style from up to 50 .md files:
   - Heading level distribution
   - Paragraph length statistics
   - Long paragraph ratio
3. **Generates** Vale configuration:
   - `.vale.ini` with StylesPath, MinAlertLevel
   - `DocsStyle/HeadingHierarchy.yml` rule
   - `Custom/Terminology.yml` rule
4. **Registers** templates found in the input
5. **Writes** knowledge base configuration to `knowledge/config.yaml`

### Step 3: Verify the KB

```bash
doc-solution check --target ./customer-inputs/docs/
```

The check uses the KB's Vale configuration to validate documents against customer style.

## KB Directory Structure

After building, the KB looks like this:

```
knowledge/
  |-- config.yaml                      # Main configuration (AI Agent readable)
  |-- rules/
  |   |-- vale/
  |   |   |-- .vale.ini                # Vale configuration
  |   |   +-- styles/
  |   |       |-- DocsStyle/
  |   |       |   +-- HeadingHierarchy.yml
  |   |       +-- Custom/
  |   |           +-- Terminology.yml
  |   +-- custom/
  |       |-- format-rules.yaml        # Custom format rules
  |       +-- structure-rules.yaml     # Custom structure rules
  |-- templates/
  |   |-- api-ref/
  |   |   +-- template.md.j2
  |   +-- dev-guide/
  |       +-- template.md.j2
  |-- glossary/
  |   |-- terms.yaml                   # Terminology definitions
  |   +-- abbreviations.yaml           # Abbreviation list
  |-- checklist/
  |   |-- quality-checklist.yaml       # Quality check items
  |   +-- review-checklist.yaml        # Review check items
  +-- meta/
      +-- style-profile.yaml           # Document style analysis
```

## Configuration Reference

### config.yaml

```yaml
# Example generated config.yaml
customer:
  name: "Huawei-HarmonyOS"
  version: "1.0.0"
  created: "2026-06-02"

rules:
  vale:
    enabled: true
    config_path: "rules/vale/.vale.ini"
    styles_path: "rules/vale/styles/"
  custom:
    format_rules: "rules/custom/format-rules.yaml"
    structure_rules: "rules/custom/structure-rules.yaml"

templates:
  api-ref:
    path: "templates/api-ref/"
    primary: "template.md.j2"
  dev-guide:
    path: "templates/dev-guide/"
    primary: "template.md.j2"

glossary:
  terms_file: "glossary/terms.yaml"
  abbreviations_file: "glossary/abbreviations.yaml"

checklist:
  quality: "checklist/quality-checklist.yaml"
  review: "checklist/review-checklist.yaml"
```

### style-profile.yaml

```yaml
# Example generated style profile
file_count: 15
analyzed_count: 15
heading_stats:
  average_per_file: 8.3
  level_distribution:
    1: 15
    2: 42
    3: 58
    4: 10
paragraph_stats:
  long_paragraph_ratio: 12.5
```

## How to Use the KB

### For Document Checking

The KB is automatically used when running `check`:

```bash
# Uses KB's Vale config and custom rules
doc-solution check --target ./output-docs/
```

The check command:
1. Reads `knowledge/config.yaml` for KB configuration
2. Applies KB's Vale configuration for style/format checks
3. Uses built-in rules for structure checks
4. Generates a unified report

### For Content Generation

The KB's templates are used by the `generate` command:

```bash
# Uses KB template 'api-ref'
doc-solution generate --template api-ref --params '{...}'

# Uses KB template 'dev-guide'
doc-solution generate --template dev-guide --params '{...}'
```

### For Custom Rules

Custom rules in `knowledge/rules/custom/` extend the built-in checks:

- **format-rules.yaml**: Define document format conventions (heading style, list style, etc.)
- **structure-rules.yaml**: Define required sections, document structure templates

## Multiple Customers

For multiple customers, maintain separate KB directories and use the `--output` flag:

```bash
doc-solution build-kb --input ./customer-a/ --name "CustomerA" --output ./kb-customer-a/
doc-solution build-kb --input ./customer-b/ --name "CustomerB" --output ./kb-customer-b/
```

Then reference the appropriate KB when checking:

```bash
doc-solution check --target ./customer-a-docs/ --config ./kb-customer-a/rules/vale/.vale.ini
```

## KB Maintenance

### Incremental Update

```bash
doc-solution build-kb --input ./updated-docs/ --name "CustomerName" --force
```

The `--force` flag overwrites the existing KB.

### Manual Editing

All KB files are YAML/JSON/plain text. Edit them directly to:

- Add new terms to `glossary/terms.yaml`
- Modify Vale rules in `rules/vale/styles/`
- Add new templates to `templates/`
- Update quality checklists in `checklist/`
