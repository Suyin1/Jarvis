# CLI Tools Reference

> Complete reference for the Doc Solution CLI tools

---

## Installation

```bash
# From project root
pip install -e .

# Verify
doc-solution --help
```

Or run directly without installation:

```bash
python -m tools.cli --help
```

## Global Options

```
--help      Show help message
--version   Show version (0.1.0)
```

## Command: check

Run quality checks on documents/code.

### Usage

```bash
doc-solution check --target <path> [options]
```

### Options

| Option | Alias | Required | Default | Description |
|--------|-------|----------|---------|-------------|
| `--target` | `-t` | Yes | - | Target file or directory |
| `--check-type` | `-c` | No | `all` | Check type: `all`, `structure`, `format`, `style` |
| `--output` | `-o` | No | `text` | Output format: `text`, `json` |
| `--vale-bin` | - | No | `vale` | Vale executable path |
| `--config` | - | No | - | Vale config file path |
| `--save-report` | - | No | - | Save JSON report to file |

### Check Types

| Type | Checks | Depends on |
|------|--------|-----------|
| `structure` | Heading hierarchy, required sections | MDParser (built-in) |
| `format` | Paragraph length, code block language, Vale style | Vale (optional) |
| `style` | Terminology, brand names, Vale style rules | Vale (optional) |
| `all` | Everything above | Both |

### Examples

```bash
# Basic check on a directory
doc-solution check --target ./knowledge/templates/

# Check a single file
doc-solution check --target ./docs/api-reference.md

# Structure-only check
doc-solution check --target ./docs/ --check-type structure

# JSON output (for programmatic consumption)
doc-solution check --target ./docs/ --output json

# JSON output saved to file
doc-solution check --target ./docs/ --output json --save-report report.json

# With custom Vale config
doc-solution check --target ./docs/ --config ./knowledge/rules/vale/.vale.ini
```

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | All checks passed (no errors) |
| 1 | Errors found or target not found |

## Command: generate

Generate document content from Jinja2 templates.

### Usage

```bash
doc-solution generate --template <name> --params <json> [options]
```

### Options

| Option | Alias | Required | Default | Description |
|--------|-------|----------|---------|-------------|
| `--template` | `-t` | Yes | - | Template name or file path |
| `--params` | `-p` | No | `{}` | Template params (JSON) |
| `--template-dir` | - | No | `knowledge/templates` | Template search directory |
| `--output` | `-o` | No | stdout | Output file path |
| `--auto-check` | - | No | true | Auto-run quality check |

### Template Resolution

The `--template` value is resolved in this order:

1. If it's a file path, use that file directly
2. If it's a directory name in `--template-dir`, use the `.j2` file inside
3. Otherwise, append `.md.j2` and search in `--template-dir`

### Examples

```bash
# Generate from named template (prints to stdout)
doc-solution generate --template api-ref --params '{"api_name": "startAbility"}'

# Generate and save to file
doc-solution generate --template api-ref --params '{...}' --output ./output/api.md

# Use custom template directory
doc-solution generate --template custom-template --template-dir ./my-templates/

# Use a specific template file
doc-solution generate --template ./path/to/template.md.j2 --params '{...}'

# Disable auto-check
doc-solution generate --template api-ref --params '{...}' --no-auto-check
```

### Built-in Templates

| Template | Description | Key Parameters |
|----------|-------------|----------------|
| `api-ref` | API reference doc | `api_name`, `declaration`, `parameters[]`, `return_type`, `error_codes[]` |
| `dev-guide` | Development guide | `title`, `overview`, `prerequisites[]`, `steps[]` |

## Command: build-kb

Build a knowledge base from customer source materials.

### Usage

```bash
doc-solution build-kb --input <dir> --name <name> [options]
```

### Options

| Option | Alias | Required | Default | Description |
|--------|-------|----------|---------|-------------|
| `--input` | `-i` | Yes | - | Source material directory |
| `--name` | `-n` | Yes | - | Customer name |
| `--output` | `-o` | No | `knowledge` | Output directory |
| `--force` | - | No | false | Overwrite existing KB |

### Examples

```bash
# Basic build
doc-solution build-kb --input ./customer-inputs/ --name "Huawei-HarmonyOS"

# Specify output directory
doc-solution build-kb --input ./customer-inputs/ --name "Huawei" --output ./kb-huawei/

# Overwrite existing KB
doc-solution build-kb --input ./customer-inputs/ --name "Huawei" --force
```

### Output

The command creates:

```
<output>/
  |-- config.yaml               # KB configuration
  |-- rules/vale/.vale.ini      # Vale config
  |-- rules/vale/styles/        # Vale style rules
  |-- rules/custom/             # Custom rule templates
  |-- templates/                # Registered templates
  |-- glossary/terms.yaml       # Empty terms file
  |-- checklist/                # Empty checklist files
  +-- meta/style-profile.yaml   # Style analysis
```

## Command: doc-solution-mcp

Run the MCP server in stdio mode (for AI Agent integration).

```bash
doc-solution-mcp
# or
python -m mcp.server
```

See `docs/mcp-server.md` for details.
