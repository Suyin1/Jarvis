"""doc-solution build-kb — knowledge base build command

Analyzes customer source materials (docs/test standards/templates),
generates a customer-specific knowledge base (rules/templates/glossary/checklist).
"""

import shutil
from datetime import date
from pathlib import Path

import click
import yaml

from engine.parser.md_parser import MDParser


def run_build_kb(input_dir, name, output="knowledge", force=False):
    """Build knowledge base from customer input and return result info.

    This is the programmatic API used by both CLI and MCP Server.
    """
    input_path = Path(input_dir)
    if not input_path.exists():
        raise ValueError("Input directory not found: %s" % input_dir)

    output_path = Path(output)
    if output_path.exists() and not force:
        raise ValueError("Output directory '%s' already exists (use --force to overwrite)" % output)

    if output_path.exists():
        shutil.rmtree(output_path)
    _create_knowledge_structure(output_path)

    input_files = _scan_input_files(input_path)
    style_profile = _analyze_docs_style(input_files)
    _generate_vale_config(output_path, name)

    config = {
        "customer": {
            "name": name,
            "version": "1.0.0",
            "created": date.today().isoformat(),
        },
        "rules": {
            "vale": {
                "enabled": True,
                "config_path": "rules/vale/.vale.ini",
                "styles_path": "rules/vale/styles/",
            },
            "custom": {
                "format_rules": "rules/custom/format-rules.yaml",
                "structure_rules": "rules/custom/structure-rules.yaml",
            },
        },
        "templates": {},
        "glossary": {
            "terms_file": "glossary/terms.yaml",
            "abbreviations_file": "glossary/abbreviations.yaml",
        },
        "checklist": {
            "quality": "checklist/quality-checklist.yaml",
            "review": "checklist/review-checklist.yaml",
        },
    }

    template_dirs = list(input_path.rglob("templates"))
    if not template_dirs:
        template_dirs = [d for d in input_path.iterdir() if d.is_dir()]
    for td in template_dirs:
        j2_files = list(td.rglob("*.j2"))
        if j2_files:
            tname = td.name
            config["templates"][tname] = {
                "path": "templates/%s/" % tname,
                "primary": j2_files[0].name,
            }

    config_path = output_path / "config.yaml"
    with open(config_path, "w", encoding="utf-8") as f:
        yaml.dump(config, f, allow_unicode=True, sort_keys=False)

    style_path = output_path / "meta" / "style-profile.yaml"
    with open(style_path, "w", encoding="utf-8") as f:
        yaml.dump(style_profile, f, allow_unicode=True, sort_keys=False)

    return {
        "name": name,
        "output_dir": str(output_path),
        "config_file": str(config_path),
        "style_profile": str(style_path),
        "file_count": len(input_files),
        "template_count": len(config["templates"]),
    }


@click.command(name="build-kb")
@click.option(
    "--input", "-i",
    "input_dir",
    required=True,
    help="Customer source material directory",
)
@click.option(
    "--name", "-n",
    required=True,
    help="Customer name (e.g. Huawei-HarmonyOS)",
)
@click.option(
    "--output", "-o",
    default="knowledge",
    help="Knowledge base output directory (default: knowledge)",
    show_default=True,
)
@click.option(
    "--force",
    is_flag=True,
    default=False,
    help="Overwrite existing knowledge base",
)
def build_kb_command(input_dir, name, output, force):
    """Build knowledge base from customer input"""
    try:
        info = run_build_kb(input_dir, name, output, force)
    except ValueError as e:
        click.echo("Error: %s" % e, err=True)
        raise click.Abort()

    click.echo("")
    click.echo("Knowledge base built: %s" % info["output_dir"])
    click.echo("  Config: %s" % info["config_file"])
    click.echo("  Style profile: %s" % info["style_profile"])
    click.echo("  Files analyzed: %d" % info["file_count"])


def _create_knowledge_structure(path):
    dirs = [
        "rules/vale/styles/DocsStyle",
        "rules/vale/styles/Custom",
        "rules/custom",
        "templates",
        "glossary",
        "checklist",
        "meta",
    ]
    for d in dirs:
        (path / d).mkdir(parents=True, exist_ok=True)


def _scan_input_files(input_path):
    extensions = {".md", ".yaml", ".yml", ".json", ".py", ".ts", ".d.ts", ".txt", ".j2"}
    files = []
    for f in input_path.rglob("*"):
        if f.is_file() and f.suffix in extensions:
            files.append(f)
    return files


def _analyze_docs_style(files):
    md_files = [f for f in files if f.suffix == ".md"]
    parser = MDParser()
    total_headings = 0
    heading_levels = {}
    total_paras = 0
    long_paras = 0
    for mf in md_files[:50]:
        try:
            content = mf.read_text(encoding="utf-8")
            structure = parser.parse(content)
            total_headings += len(structure.headings)
            for h in structure.headings:
                heading_levels[h.level] = heading_levels.get(h.level, 0) + 1
            for line in content.split("\n"):
                stripped = line.strip()
                if stripped and len(stripped) > 200:
                    long_paras += 1
                if stripped:
                    total_paras += 1
        except Exception:
            pass
    return {
        "file_count": len(md_files),
        "analyzed_count": min(len(md_files), 50),
        "heading_stats": {
            "average_per_file": round(total_headings / max(len(md_files[:50]), 1), 1),
            "level_distribution": heading_levels,
        },
        "paragraph_stats": {
            "long_paragraph_ratio": round(
                long_paras / max(total_paras, 1) * 100, 1
            ),
        },
    }


def _generate_vale_config(output_path, customer_name):
    ini_content = """; Vale Configuration - %s
; Generated by Doc Solution Knowledge Builder

StylesPath = styles

MinAlertLevel = suggestion

[*.md]
BasedOnStyles = DocsStyle, Custom

[DocsStyle]
HeadingHierarchy = YES
SentenceLength = YES

[Custom]
Terminology = YES
""" % customer_name

    ini_path = output_path / "rules" / "vale" / ".vale.ini"
    ini_path.write_text(ini_content, encoding="utf-8")

    heading_rule = """extends: existence
message: "Heading level should use '#' markers"
level: warning
scope: heading
nonword: true
tokens:
  - '#+ '
"""
    (output_path / "rules" / "vale" / "styles" / "DocsStyle" / "HeadingHierarchy.yml").write_text(
        heading_rule, encoding="utf-8"
    )

    term_rule = """extends: substitution
message: "Use '%%s' instead of '%%s'"
level: error
ignorecase: true
swap:
  api: API
  sdk: SDK
  ui: UI
  json: JSON
  yaml: YAML
  cli: CLI
  rest: REST
  http: HTTP
  ssh: SSH
  mcp: MCP
"""
    (output_path / "rules" / "vale" / "styles" / "Custom" / "Terminology.yml").write_text(
        term_rule, encoding="utf-8"
    )
