"""Markdown 解析器测试"""

from engine.parser.md_parser import MDParser


def test_parse_headings():
    parser = MDParser()
    content = "# Title\n\n## Section 1\n\n### Subsection\n\n## Section 2\n"
    structure = parser.parse(content)
    assert len(structure.headings) == 4
    assert structure.headings[0].level == 1
    assert structure.headings[0].text == "Title"
    assert structure.headings[1].level == 2
    assert structure.headings[1].text == "Section 1"


def test_parse_code_blocks():
    parser = MDParser()
    content = "text\n```python\nprint('hello')\n```\nmore text\n```\nno lang\n```\n"
    structure = parser.parse(content)
    assert len(structure.code_blocks) == 2
    assert structure.code_blocks[0].language == "python"
    assert structure.code_blocks[1].language is None


def test_parse_links():
    parser = MDParser()
    content = "see [Google](https://google.com) and [Docs](./docs.md)"
    structure = parser.parse(content)
    assert len(structure.links) == 2
    assert structure.links[0].text == "Google"
    assert structure.links[0].url == "https://google.com"


def test_check_heading_hierarchy_no_h1():
    parser = MDParser()
    content = "## Section 1\n\n### Sub\n"
    structure = parser.parse(content)
    issues = parser.check_heading_hierarchy(structure)
    assert any(i["rule"] == "heading-hierarchy" for i in issues)
    assert any("缺少一级标题" in i["message"] for i in issues)


def test_check_heading_hierarchy_skip_level():
    parser = MDParser()
    content = "# Title\n\n### Jump to H3\n"
    structure = parser.parse(content)
    issues = parser.check_heading_hierarchy(structure)
    assert any("跳级" in i["message"] for i in issues)


def test_check_required_sections():
    parser = MDParser()
    content = "# Title\n\n## 简介\n\n## 开发指导\n"
    structure = parser.parse(content)
    issues = parser.check_required_sections(
        structure, ["简介", "开发指导", "API参考", "常见问题"]
    )
    assert len(issues) == 2
    assert any("API参考" in i["message"] for i in issues)
    assert any("常见问题" in i["message"] for i in issues)


def test_word_count():
    parser = MDParser()
    content = "hello world\n\nfoo bar baz\n"
    structure = parser.parse(content)
    assert structure.word_count == 5


def test_empty_doc():
    parser = MDParser()
    structure = parser.parse("")
    assert len(structure.headings) == 0
    assert len(structure.code_blocks) == 0
    assert len(structure.links) == 0
    assert structure.word_count == 0
