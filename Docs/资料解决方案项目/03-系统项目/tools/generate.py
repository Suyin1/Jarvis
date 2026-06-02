"""doc-solution generate — content generation command

Generates document content from templates using Jinja2.
Optionally runs quality check after generation.
"""

import json
from pathlib import Path

import click
import yaml
from jinja2 import Environment, FileSystemLoader, TemplateNotFound

from engine.checker.reporter import CheckReport, CheckResult, ReportItem, TraceStep


def run_generate(template, params_dict, template_dir="knowledge/templates",
                 output=None, auto_check=True):
    """Generate content from template and return (content, report_or_none).

    This is the programmatic API used by both CLI and MCP Server.
    """
    template_dir_path = Path(template_dir)
    template_name = ""

    if Path(template).exists():
        template_file = Path(template)
        template_dir_path = template_file.parent
        template_name = template_file.name
    elif (template_dir_path / template).is_dir():
        td = template_dir_path / template
        j2_files = list(td.glob("*.j2"))
        if j2_files:
            template_dir_path = td
            template_name = j2_files[0].name
        else:
            raise ValueError("Template directory '%s' has no .j2 files" % template)
    else:
        template_name = "%s.md.j2" % template

    if not template_dir_path.exists():
        raise ValueError("Template directory not found: %s" % template_dir_path)

    env = Environment(
        loader=FileSystemLoader(str(template_dir_path)),
        autoescape=False,
    )

    try:
        tmpl = env.get_template(template_name)
    except TemplateNotFound:
        raise ValueError("Template not found: %s in %s" % (template_name, template_dir_path))

    try:
        content = tmpl.render(**params_dict)
    except Exception as e:
        raise ValueError("Template render failed: %s" % e)

    if output:
        output_path = Path(output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding="utf-8")

    report = None
    if auto_check:
        from engine.parser.md_parser import MDParser
        parser = MDParser()
        structure = parser.parse(content)
        issues = parser.check_heading_hierarchy(structure)

        result = CheckResult()
        for i in issues:
            result.items.append(ReportItem(
                rule_id=i["rule"],
                rule_name=i["rule"],
                severity=i["severity"],
                status="failed",
                message=i["message"],
                line=i["line"],
            ))

        report = CheckReport(
            check_id="gen-check-%d" % (id(content) % 1000000),
            check_type="structure",
            target=output or "(stdout)",
            result=result,
        )

    return content, report


@click.command(name="generate")
@click.option(
    "--template", "-t",
    required=True,
    help="Template name (e.g. api-ref, dev-guide) or template file path",
)
@click.option(
    "--params", "-p",
    default="{}",
    help="Template parameters (JSON string)",
)
@click.option(
    "--template-dir",
    default="knowledge/templates",
    help="Template directory path",
    show_default=True,
)
@click.option(
    "--output", "-o",
    help="Output file path (omit to print to stdout)",
)
@click.option(
    "--auto-check",
    is_flag=True,
    default=True,
    help="Auto run quality check after generation",
)
def generate_command(template, params, template_dir, output, auto_check):
    """Generate document content from template"""
    try:
        params_dict = json.loads(params)
    except json.JSONDecodeError as e:
        click.echo("Error: params is not valid JSON: %s" % e, err=True)
        raise click.Abort()

    try:
        content, report = run_generate(template, params_dict, template_dir, output, auto_check)
    except ValueError as e:
        click.echo("Error: %s" % e, err=True)
        raise click.Abort()

    if not output:
        click.echo(content)
    else:
        click.echo("Content generated: %s" % output)

    if report:
        click.echo("")
        click.echo(report.to_text())


def _list_templates(template_dir):
    files = list(template_dir.rglob("*.j2"))
    if not files:
        return "(empty)"
    return "\n      ".join(f.name for f in files)
