"""Markdown 文档解析器

提取文档结构（标题层级、章节、代码块、链接等），
为结构检查和内容分析提供基础。
"""

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional


@dataclass
class HeadingInfo:
    level: int
    text: str
    line_number: int
    anchor: str = ""


@dataclass
class CodeBlock:
    language: Optional[str]
    content: str
    start_line: int
    end_line: int


@dataclass
class LinkInfo:
    text: str
    url: str
    line_number: int
    is_valid: Optional[bool] = None


@dataclass
class DocStructure:
    headings: List[HeadingInfo] = field(default_factory=list)
    code_blocks: List[CodeBlock] = field(default_factory=list)
    links: List[LinkInfo] = field(default_factory=list)
    total_lines: int = 0
    word_count: int = 0


class MDParser:
    """Markdown 文档解析器"""

    HEADING_PATTERN = re.compile(r"^(#{1,6})\s+(.+)$", re.MULTILINE)
    CODE_BLOCK_PATTERN = re.compile(r"```(\w*)\n(.*?)```", re.DOTALL)
    LINK_PATTERN = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
    WORD_PATTERN = re.compile(r"\b\w+\b")

    def parse(self, content: str) -> DocStructure:
        """解析 Markdown 内容，返回文档结构"""
        lines = content.split("\n")
        structure = DocStructure(
            total_lines=len(lines),
            word_count=len(self.WORD_PATTERN.findall(content)),
        )

        structure.headings = self._parse_headings(content, lines)
        structure.code_blocks = self._parse_code_blocks(content)
        structure.links = self._parse_links(content, lines)

        return structure

    def parse_file(self, file_path: str) -> DocStructure:
        """解析 Markdown 文件"""
        path = Path(file_path)
        if not path.exists():
            raise FileNotFoundError(f"文件不存在: {file_path}")
        content = path.read_text(encoding="utf-8")
        return self.parse(content)

    def _parse_headings(self, content: str, lines: List[str]) -> List[HeadingInfo]:
        """提取标题层级"""
        headings = []
        for match in self.HEADING_PATTERN.finditer(content):
            level = len(match.group(1))
            text = match.group(2).strip()
            # 计算行号
            line_number = content[:match.start()].count("\n") + 1
            anchor = text.lower().replace(" ", "-").replace(".", "")
            headings.append(HeadingInfo(
                level=level,
                text=text,
                line_number=line_number,
                anchor=anchor,
            ))
        return headings

    def _parse_code_blocks(self, content: str) -> List[CodeBlock]:
        """提取代码块"""
        blocks = []
        for match in self.CODE_BLOCK_PATTERN.finditer(content):
            language = match.group(1).strip() or None
            code_content = match.group(2)
            start_line = content[:match.start()].count("\n") + 1
            end_line = start_line + match.group(0).count("\n")
            blocks.append(CodeBlock(
                language=language,
                content=code_content,
                start_line=start_line,
                end_line=end_line,
            ))
        return blocks

    def _parse_links(self, content: str, lines: List[str]) -> List[LinkInfo]:
        """提取链接"""
        links = []
        for match in self.LINK_PATTERN.finditer(content):
            line_number = content[:match.start()].count("\n") + 1
            links.append(LinkInfo(
                text=match.group(1),
                url=match.group(2),
                line_number=line_number,
            ))
        return links

    def check_heading_hierarchy(self, structure: DocStructure) -> List[dict]:
        """检查标题层级是否规范（h1 -> h2 -> h3...，不允许跳级）"""
        issues = []
        if not structure.headings:
            return issues

        # 检查是否有且仅有一个 h1
        h1s = [h for h in structure.headings if h.level == 1]
        if len(h1s) == 0:
            issues.append({
                "rule": "heading-hierarchy",
                "severity": "error",
                "message": "文档缺少一级标题 (H1)",
                "line": 1,
            })
        elif len(h1s) > 1:
            for h in h1s[1:]:
                issues.append({
                    "rule": "heading-hierarchy",
                    "severity": "error",
                    "message": f"文档有多个一级标题: '{h.text}'",
                    "line": h.line_number,
                })

        # 检查标题层级是否跳级
        prev_level = 1
        for heading in structure.headings:
            if heading.level == 1:
                prev_level = 1
                continue
            if heading.level > prev_level + 1:
                issues.append({
                    "rule": "heading-hierarchy",
                    "severity": "warning",
                    "message": (
                        f"标题层级跳级: 从 H{prev_level} 到 H{heading.level} "
                        f"('{heading.text}')"
                    ),
                    "line": heading.line_number,
                })
            prev_level = heading.level

        return issues

    def check_required_sections(
        self,
        structure: DocStructure,
        required: List[str],
    ) -> List[dict]:
        """检查是否包含所有必选章节"""
        heading_texts = [h.text for h in structure.headings]
        issues = []
        for section in required:
            found = any(section in text for text in heading_texts)
            if not found:
                issues.append({
                    "rule": "required-sections",
                    "severity": "error",
                    "message": f"缺少必选章节: '{section}'",
                    "line": structure.total_lines,
                })
        return issues
