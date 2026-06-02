"""doc-solution check — 质量检查命令

对文档/代码执行质量检查，支持格式/风格/结构/代码四种检查类型。
检查结果输出为结构化报告（JSON/文本）。
"""

import json
import time
from pathlib import Path
from typing import List, Optional

import click

from engine.checker.reporter import CheckReport, CheckResult, ReportItem, TraceStep
from engine.parser.md_parser import MDParser
from engine.rule_engine.vale_adapter import ValeAdapter, ValeConfig


@click.command(name="check")
@click.option(
    "--target", "-t",
    required=True,
    help="待检查的文件或目录路径",
)
@click.option(
    "--check-type", "-c",
    type=click.Choice(["all", "format", "style", "structure", "code", "consistency"]),
    default="all",
    help="检查类型",
)
@click.option(
    "--output", "-o",
    type=click.Choice(["json", "text"]),
    default="text",
    help="输出格式",
)
@click.option(
    "--vale-bin",
    default="vale",
    help="Vale 可执行文件路径",
)
@click.option(
    "--config",
    "config_path",
    help="Vale 配置文件路径 (.vale.ini)",
)
@click.option(
    "--save-report",
    help="将报告保存到指定文件路径",
)
def check_command(target, check_type, output, vale_bin, config_path, save_report):
    """对文档/代码执行质量检查"""
    target_path = Path(target)
    if not target_path.exists():
        click.echo("错误: 目标路径不存在: %s" % target, err=True)
        raise click.Abort()

    result = CheckResult()

    is_json = output == "json"
    if not is_json:
        click.echo("开始检查: %s" % target)
        click.echo("  检查类型: %s" % check_type)
        click.echo("")

    # 结构检查
    if check_type in ("all", "structure"):
        t0 = time.time()
        if not is_json:
            click.echo("  [1/3] 执行结构检查...", nl=False)
        structure_items = _run_structure_check(target_path)
        result.items.extend(structure_items)
        result.trace.append(TraceStep(
            step="结构检查", tool="md_parser",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))
        if not is_json:
            click.echo(" 完成 (%d 项)" % len(structure_items))

    # Vale 检查
    if check_type in ("all", "format", "style"):
        t0 = time.time()
        if not is_json:
            click.echo("  [2/3] 执行 Vale 检查...", nl=False)
        vale_items = _run_vale_check(target_path, vale_bin, config_path)
        result.items.extend(vale_items)
        result.trace.append(TraceStep(
            step="Vale检查", tool="vale",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))
        if not is_json:
            click.echo(" 完成 (%d 项)" % len(vale_items))

    # 内置格式检查
    if check_type in ("all", "format"):
        t0 = time.time()
        if not is_json:
            click.echo("  [3/3] 执行格式检查...", nl=False)
        format_items = _run_format_check(target_path)
        result.items.extend(format_items)
        result.trace.append(TraceStep(
            step="格式检查", tool="builtin",
            status="completed",
            duration_ms=int((time.time() - t0) * 1000),
        ))
        if not is_json:
            click.echo(" 完成 (%d 项)" % len(format_items))

    report = CheckReport(
        check_id="check-%d" % int(time.time()),
        check_type=check_type,
        target=str(target),
        result=result,
    )

    click.echo("")

    if output == "json":
        click.echo(report.to_json())
    else:
        click.echo(report.to_text())

    if save_report:
        save_path = Path(save_report)
        save_path.parent.mkdir(parents=True, exist_ok=True)
        save_path.write_text(report.to_json(), encoding="utf-8")
        click.echo("报告已保存: %s" % save_path)

    if result.errors:
        raise click.Abort()


def _run_structure_check(target_path: Path) -> List[ReportItem]:
    """执行文档结构检查"""
    items = []
    parser = MDParser()

    md_files = _find_md_files(target_path)
    for md_file in md_files:
        try:
            structure = parser.parse_file(str(md_file))
        except Exception as e:
            items.append(ReportItem(
                rule_id="parse-error",
                rule_name="文件解析错误",
                severity="error",
                status="failed",
                message=f"解析失败: {e}",
                file=str(md_file),
            ))
            continue

        # 标题层级检查
        hierarchy_issues = parser.check_heading_hierarchy(structure)
        for issue in hierarchy_issues:
            items.append(ReportItem(
                rule_id=issue["rule"],
                rule_name="标题层级规范",
                severity=issue["severity"],
                status="failed",
                message=issue["message"],
                file=str(md_file),
                line=issue["line"],
            ))

    if not md_files:
        items.append(ReportItem(
            rule_id="no-md-files",
            rule_name="未找到Markdown文件",
            severity="warning",
            status="failed",
            message=f"目标路径中未找到 .md 文件: {target_path}",
        ))

    return items


def _run_vale_check(target_path: Path, vale_bin: str, config_path: Optional[str]) -> List[ReportItem]:
    """执行 Vale 规则检查"""
    items = []
    config = ValeConfig(vale_bin=vale_bin, config_path=config_path)
    adapter = ValeAdapter(config)

    vale_result = adapter.check(str(target_path))
    if vale_result.error_message and vale_result.exit_code == -1:
        items.append(ReportItem(
            rule_id="vale-not-found",
            rule_name="Vale 未就绪",
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
                suggestion=f"匹配内容: {alert.match}" if alert.match else "",
            ))

    return items


def _run_format_check(target_path: Path) -> List[ReportItem]:
    """执行内置格式检查（不依赖 Vale 的基础检查）"""
    items = []
    parser = MDParser()

    md_files = _find_md_files(target_path)
    for md_file in md_files:
        content = md_file.read_text(encoding="utf-8")
        structure = parser.parse(content)

        # 代码块语言标注检查
        for block in structure.code_blocks:
            if not block.language:
                items.append(ReportItem(
                    rule_id="code-block-language",
                    rule_name="代码块语言标注",
                    severity="warning",
                    status="failed",
                    message="代码块缺少语言标注",
                    file=str(md_file),
                    line=block.start_line,
                    suggestion="添加语言标注，例如: ```python",
                ))

        # 段落长度检查（段落不超过200字）
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
                    rule_name="段落长度",
                    severity="warning",
                    status="failed",
                    message=f"段落超过200字 (当前 {para_chars} 字)",
                    file=str(md_file),
                    line=para_start,
                    suggestion="考虑拆分段落，每段不超过200字",
                ))

    return items


def _find_md_files(path: Path) -> List[Path]:
    """递归查找所有 .md 文件"""
    if path.is_file():
        return [path] if path.suffix == ".md" else []
    return list(path.rglob("*.md"))
