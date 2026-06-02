"""doc-solution generate — 内容生成命令

基于模板和参数生成文档内容，生成后自动执行质量检查。
使用 Jinja2 模板引擎渲染。
"""

import json
from pathlib import Path

import click
import yaml
from jinja2 import Environment, FileSystemLoader, TemplateNotFound

from engine.checker.reporter import CheckReport, CheckResult, ReportItem, TraceStep


@click.command(name="generate")
@click.option(
    "--template", "-t",
    required=True,
    help="模板名称 (如 api-ref, dev-guide)，或模板文件路径",
)
@click.option(
    "--params", "-p",
    default="{}",
    help="模板参数 (JSON 字符串)",
)
@click.option(
    "--template-dir",
    default="knowledge/templates",
    help="模板目录路径",
    show_default=True,
)
@click.option(
    "--output", "-o",
    help="输出文件路径（不指定则输出到终端）",
)
@click.option(
    "--auto-check",
    is_flag=True,
    default=True,
    help="生成后自动进行质量检查",
)
def generate_command(template, params, template_dir, output, auto_check):
    """基于模板生成文档内容"""
    try:
        params_dict = json.loads(params)
    except json.JSONDecodeError as e:
        click.echo("错误: 参数不是有效的 JSON: %s" % e, err=True)
        raise click.Abort()

    template_dir_path = Path(template_dir)
    template_name = ""

    # 判断 template 是文件路径、目录名还是模板名
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
            click.echo("错误: 模板目录 '%s' 中没有模板文件 (.j2)" % template, err=True)
            raise click.Abort()
    else:
        template_name = "%s.md.j2" % template

    if not template_dir_path.exists():
        click.echo("错误: 模板目录不存在: %s" % template_dir_path, err=True)
        raise click.Abort()

    env = Environment(
        loader=FileSystemLoader(str(template_dir_path)),
        autoescape=False,
    )

    try:
        tmpl = env.get_template(template_name)
    except TemplateNotFound:
        click.echo("错误: 模板未找到: %s" % template_name, err=True)
        click.echo("      在目录: %s" % template_dir_path)
        click.echo("      可用的模板: %s" % _list_templates(template_dir_path))
        raise click.Abort()

    try:
        content = tmpl.render(**params_dict)
    except Exception as e:
        click.echo("错误: 模板渲染失败: %s" % e, err=True)
        raise click.Abort()

    if output:
        output_path = Path(output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding="utf-8")
        click.echo("内容已生成: %s" % output_path)
    else:
        click.echo(content)

    if auto_check:
        click.echo("")
        click.echo("执行质量检查...")
        from tools.check import check_command as run_check
        check_target = output or click.get_text_stream("stdin")
        # 简单检查：结构分析
        from engine.parser.md_parser import MDParser
        parser = MDParser()
        structure = parser.parse(content)
        issues = parser.check_heading_hierarchy(structure)

        result = CheckResult()
        result.items.extend(ReportItem(
            rule_id=i["rule"],
            rule_name=i["rule"],
            severity=i["severity"],
            status="failed",
            message=i["message"],
            line=i["line"],
        ) for i in issues)

        report = CheckReport(
            check_id=f"gen-check-{id(content)}",
            check_type="structure",
            target=output or "(stdout)",
            result=result,
        )
        click.echo(report.to_text())


def _list_templates(template_dir: Path) -> str:
    files = list(template_dir.rglob("*.j2"))
    if not files:
        return "(空)"
    return "\n      ".join(f.name for f in files)
