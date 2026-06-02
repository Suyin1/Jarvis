"""doc-solution check — quality check command

Performs quality checks on documents/code.
Supports format/style/structure/code check types.
Outputs structured reports (JSON/text).
"""

import json
import time
from pathlib import Path

import click

from engine.checker.reporter import CheckReport, CheckResult, ReportItem, TraceStep
from engine.parser.md_parser import MDParser
from engine.rule_engine.vale_adapter import ValeAdapter, ValeConfig


def run_check(target, check_type="all", output_format="text",
              vale_bin="vale", config_path=None, save_report=None):
    """Run quality check and return a CheckReport.

    This is the programmatic API used by both CLI and MCP Server.
    """
    target_path = Path(target)
    if not target_path.exists():
        raise ValueError("Target path does not exist: %s" % target)

    result = CheckResult()

    # Structure check
    if check_type in ("all", "structure"):
        t0 = time.time()
        structure_items = _run_structure_check(target_path)
        result.items.extend(structure_items)
        result.trace.append(TraceStep(
            step="StructureCheck", tool="md_parser",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))

    # Vale check
    if check_type in ("all", "format", "style"):
        t0 = time.time()
        vale_items = _run_vale_check(target_path, vale_bin, config_path)
        result.items.extend(vale_items)
        result.trace.append(TraceStep(
            step="ValeCheck", tool="vale",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))

    # Built-in format check
    if check_type in ("all", "format"):
        t0 = time.time()
        format_items = _run_format_check(target_path)
        result.items.extend(format_items)
        result.trace.append(TraceStep(
            step="FormatCheck", tool="builtin",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))

    report = CheckReport(
        check_id="check-%d" % int(time.time()),
        check_type=check_type,
        target=str(target),
        result=result,
    )

    if save_report:
        save_path = Path(save_report)
        save_path.parent.mkdir(parents=True, exist_ok=True)
        save_path.write_text(report.to_json(), encoding="utf-8")

    return report


@click.command(name="check")
@click.option(
    "--target", "-t",
    required=True,
    help="Target file or directory path",
)
@click.option(
    "--check-type", "-c",
    type=click.Choice(["all", "format", "style", "structure", "code", "consistency"]),
    default="all",
    help="Check type",
)
@click.option(
    "--output", "-o",
    type=click.Choice(["json", "text"]),
    default="text",
    help="Output format",
)
@click.option(
    "--vale-bin",
    default="vale",
    help="Vale executable path",
)
@click.option(
    "--config",
    "config_path",
    help="Vale config file path (.vale.ini)",
)
@click.option(
    "--save-report",
    help="Save report to file path",
)
def check_command(target, check_type, output, vale_bin, config_path, save_report):
    """Run quality check on documents/code"""
    is_json = output == "json"

    try:
        report = run_check(target, check_type, output, vale_bin, config_path, save_report)
    except ValueError as e:
        click.echo("Error: %s" % e, err=True)
        raise click.Abort()

    if not is_json:
        click.echo("")
        click.echo(report.to_text())
    else:
        click.echo(report.to_json())

    if report.result.errors:
        raise click.Abort()


def _run_structure_check(target_path):
    items = []
    parser = MDParser()
    md_files = _find_md_files(target_path)
    for md_file in md_files:
        try:
            structure = parser.parse_file(str(md_file))
        except Exception as e:
            items.append(ReportItem(
                rule_id="parse-error",
                rule_name="ParseError",
                severity="error",
                status="failed",
                message="Parse failed: %s" % e,
                file=str(md_file),
            ))
            continue
        hierarchy_issues = parser.check_heading_hierarchy(structure)
        for issue in hierarchy_issues:
            items.append(ReportItem(
                rule_id=issue["rule"],
                rule_name="HeadingHierarchy",
                severity=issue["severity"],
                status="failed",
                message=issue["message"],
                file=str(md_file),
                line=issue["line"],
            ))
    if not md_files:
        items.append(ReportItem(
            rule_id="no-md-files",
            rule_name="NoMarkdownFiles",
            severity="warning",
            status="failed",
            message="No .md files found in target: %s" % target_path,
        ))
    return items


def _run_vale_check(target_path, vale_bin, config_path):
    items = []
    config = ValeConfig(vale_bin=vale_bin, config_path=config_path)
    adapter = ValeAdapter(config)
    vale_result = adapter.check(str(target_path))
    if vale_result.error_message and vale_result.exit_code == -1:
        items.append(ReportItem(
            rule_id="vale-not-found",
            rule_name="ValeNotReady",
            severity="warning",
            status="needs_review",
            message=vale_result.error_message,
        ))
        return items
    for file_path, file_result in vale_result.files.items():
        for alert in file_result.alerts:
            items.append(ReportItem(
                rule_id=alert.check,
                rule_name=alert.check,
                severity=alert.severity,
                status="failed" if alert.severity == "error" else "needs_review",
                message=alert.message,
                file=file_path,
                line=alert.line,
                column=alert.column,
                suggestion="Match: %s" % alert.match if alert.match else "",
            ))
    return items


def _run_format_check(target_path):
    items = []
    parser = MDParser()
    md_files = _find_md_files(target_path)
    for md_file in md_files:
        content = md_file.read_text(encoding="utf-8")
        structure = parser.parse(content)
        for block in structure.code_blocks:
            if not block.language:
                items.append(ReportItem(
                    rule_id="code-block-language",
                    rule_name="CodeBlockLanguage",
                    severity="warning",
                    status="failed",
                    message="Code block missing language annotation",
                    file=str(md_file),
                    line=block.start_line,
                    suggestion="Add language annotation, e.g. ```python",
                ))
        lines = content.split("\n")
        para_chars = 0
        para_start = 0
        for i, line in enumerate(lines):
            stripped = line.strip()
            if stripped == "":
                para_chars = 0
                continue
            if para_chars == 0:
                para_start = i + 1
            para_chars += len(stripped)
            if para_chars > 200 and para_chars - len(stripped) <= 200:
                items.append(ReportItem(
                    rule_id="paragraph-length",
                    rule_name="ParagraphLength",
                    severity="warning",
                    status="failed",
                    message="Paragraph exceeds 200 chars (current %d)" % para_chars,
                    file=str(md_file),
                    line=para_start,
                    suggestion="Consider splitting paragraph",
                ))
    return items


def _find_md_files(path):
    if path.is_file():
        return [path] if path.suffix == ".md" else []
    return list(path.rglob("*.md"))
