"""doc-solution build-kb — 知识库构建命令

分析客户输入的源材料（文档/测试标准/模板等），
生成客户专属的知识库（规则集/模板库/术语表/检查清单）。
"""

import shutil
from datetime import date
from pathlib import Path
from typing import List

import click
import yaml

from engine.parser.md_parser import MDParser


@click.command(name="build-kb")
@click.option(
    "--input", "-i",
    "input_dir",
    required=True,
    help="客户提供的源材料目录",
)
@click.option(
    "--name", "-n",
    required=True,
    help="客户名称 (如 华为-HarmonyOS)",
)
@click.option(
    "--output", "-o",
    default="knowledge",
    help="知识库输出目录 (默认: knowledge)",
    show_default=True,
)
@click.option(
    "--force",
    is_flag=True,
    default=False,
    help="覆盖已有知识库",
)
def build_kb_command(input_dir, name, output, force):
    """从客户输入构建知识库"""
    input_path = Path(input_dir)
    if not input_path.exists():
        click.echo("错误: 输入目录不存在: %s" % input_dir, err=True)
        raise click.Abort()

    output_path = Path(output)
    if output_path.exists() and not force:
        click.echo(
            f"错误: 输出目录已存在: {output} (使用 --force 覆盖)",
            err=True,
        )
        raise click.Abort()

    click.echo("构建知识库: %s" % name)
    click.echo("  输入: %s" % input_dir)
    click.echo("  输出: %s" % output)
    click.echo("")

    # 创建知识库目录
    if output_path.exists():
        shutil.rmtree(output_path)
    _create_knowledge_structure(output_path)

    # 扫描输入目录
    click.echo("  [1/4] 扫描输入文件...", nl=False)
    input_files = _scan_input_files(input_path)
    click.echo("  发现 %d 个文件" % len(input_files))

    # 分析源文档提取风格特征
    click.echo("  [2/4] 分析文档风格...", nl=False)
    style_profile = _analyze_docs_style(input_files)
    click.echo("  完成")

    # 生成 Vale 配置
    click.echo("  [3/4] 生成 Vale 规则配置...", nl=False)
    _generate_vale_config(output_path, name)
    click.echo("  完成")

    # 生成知识库配置文件
    click.echo("  [4/4] 生成知识库配置...", nl=False)
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

    # 扫描模板
    template_dirs = list(input_path.rglob("templates"))
    if not template_dirs:
        template_dirs = [d for d in input_path.iterdir() if d.is_dir()]
    for td in template_dirs:
        j2_files = list(td.rglob("*.j2"))
        if j2_files:
            tname = td.name
            config["templates"][tname] = {
                "path": f"templates/{tname}/",
                "primary": j2_files[0].name,
            }

    config_path = output_path / "config.yaml"
    with open(config_path, "w", encoding="utf-8") as f:
        yaml.dump(config, f, allow_unicode=True, sort_keys=False)
    click.echo("  完成")

    # 保存风格摘要
    style_path = output_path / "meta" / "style-profile.yaml"
    with open(style_path, "w", encoding="utf-8") as f:
        yaml.dump(style_profile, f, allow_unicode=True, sort_keys=False)

    click.echo("")
    click.echo("知识库已构建: %s" % output_path)
    click.echo("  配置文件: %s" % config_path)
    click.echo("  风格摘要: %s" % style_path)


def _create_knowledge_structure(path: Path):
    """创建知识库目录结构"""
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


def _scan_input_files(input_path: Path) -> List[Path]:
    """递归扫描输入目录"""
    extensions = {".md", ".yaml", ".yml", ".json", ".py", ".ts", ".d.ts", ".txt", ".j2"}
    files = []
    for f in input_path.rglob("*"):
        if f.is_file() and f.suffix in extensions:
            files.append(f)
    return files


def _analyze_docs_style(files: List[Path]) -> dict:
    """分析文档风格特征"""
    md_files = [f for f in files if f.suffix == ".md"]
    parser = MDParser()

    total_headings = 0
    heading_levels = {}
    total_paras = 0
    long_paras = 0

    for mf in md_files[:50]:  # 最多分析50个文件
        try:
            content = mf.read_text(encoding="utf-8")
            structure = parser.parse(content)

            total_headings += len(structure.headings)
            last_level = 0
            for h in structure.headings:
                heading_levels[h.level] = heading_levels.get(h.level, 0) + 1
                if h.level != 1:
                    if h.level > last_level + 1:
                        pass  # 跳级
                last_level = h.level

            # 段落分析
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


def _generate_vale_config(output_path: Path, customer_name: str):
    """生成 Vale 配置文件"""
    # .vale.ini
    ini_content = f"""# Vale Configuration - {customer_name}
# Generated by Doc Solution Knowledge Builder

StylesPath = styles

MinAlertLevel = suggestion

[*.md]
BasedOnStyles = DocsStyle, Custom

[DocsStyle]
HeadingHierarchy = YES
SentenceLength = YES

[Custom]
Terminology = YES
"""
    ini_path = output_path / "rules" / "vale" / ".vale.ini"
    ini_path.write_text(ini_content, encoding="utf-8")

    # DocsStyle/HeadingHierarchy.yml
    heading_rule = """extends: existence
message: "标题层级建议使用 '#' 标记"
level: warning
scope: heading
nonword: true
tokens:
  - '#+ '
"""
    (output_path / "rules" / "vale" / "styles" / "DocsStyle" / "HeadingHierarchy.yml").write_text(
        heading_rule, encoding="utf-8"
    )

    # Custom/Terminology.yml
    term_rule = """extends: conditional
message: "术语 '%s' 可能不符合规范"
level: suggestion
scope: text
ignorecase: true
first: '\\b(API|SDK|UI|JSON|YAML|CLI|REST|HTTP|SSH|MCP)\\b'
second: ''
action:
  name: replace
  params:
    - API
    - API
"""
    (output_path / "rules" / "vale" / "styles" / "Custom" / "Terminology.yml").write_text(
        term_rule, encoding="utf-8"
    )
