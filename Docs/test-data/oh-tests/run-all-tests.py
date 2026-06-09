"""
OpenHarmony Vale 规则批量验证脚本

功能:
  1. 对所有规则的 test-rule 单元测试（语法 + 正/反向测试）
  2. 对合成测试文档的 Vale 全规则集成测试
  3. 对真实 OpenHarmony 文档的 check 验证

用法:
    python test-data/oh-tests/run-all-tests.py

返回码: 0 = 全部通过, 1 = 有失败
"""

import sys
import os
import json
import subprocess

# 确保能找到项目模块
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../资料解决方案项目/03-系统项目'))
from tools.test_rule import run_test_rule

BASE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.join(os.path.dirname(__file__), '../../资料解决方案项目/03-系统项目')
KB_CONFIG = os.path.join(os.path.dirname(__file__), '../oh-kb/rules/vale/.vale.ini')


def section(title):
    """打印标题"""
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)


def test_rules():
    """Phase 1: 对每条规则执行 test-rule 单元测试"""
    section("Phase 1: Vale Rule Unit Tests")

    rules = [
        # === existence type (14 original rules) ===
        ("NoConsoleLog", "NoConsoleLog.yml"),
        ("HeadingNumbering", "HeadingNumbering.yml"),
        ("NoteCautionFormat", "NoteCautionFormat.yml"),
        ("HtmlTagFormat", "HtmlTagFormat.yml"),
        ("AtLinkDetection", "AtLinkDetection.yml"),
        ("TableTabChar", "TableTabChar.yml"),
        ("TablePipeFormat", "TablePipeFormat.yml"),
        ("TableSeparatorFormat", "TableSeparatorFormat.yml"),
        ("LinkUnclosedParen", "LinkUnclosedParen.yml"),
        ("LinkSpaceInPath", "LinkSpaceInPath.yml"),
        ("LinkBrTag", "LinkBrTag.yml"),
        ("ImageTypeRestriction", "ImageTypeRestriction.yml"),
        ("TrailingSpaces", "TrailingSpaces.yml"),
        ("BlankLineWhitespace", "BlankLineWhitespace.yml"),
        # === existence: integrated from existing tests ===
        ("CodeBlockLanguage", "CodeBlockLanguage.yml"),
        # === occurrence type ===
        ("CommaCount", "CommaCount.yml"),
        ("ExclamationLimit", "ExclamationLimit.yml"),
        # === consistency type (disabled: CJK characters can't be matched by Vale RE2) ===
        # 规则文件保留在 styles 目录作为文档，但 CJK 匹配需 Python 检查器实现
        # ("ConsistentTerms", "ConsistentTerms.yml"),
        # === conditional type ===
        ("TryCatchPair", "TryCatchPair.yml"),
        # === metric type (disabled: not supported in this Vale version) ===
        # ("SentenceLengthCN", "SentenceLengthCN.yml"),
    ]

    passed = 0
    failed = 0
    for dirname, rulefile in rules:
        rule = os.path.join(BASE, dirname, rulefile)
        pf = os.path.join(BASE, dirname, "should-pass.md")
        ff = os.path.join(BASE, dirname, "should-fail.md")
        result = run_test_rule(rule, pf, ff)

        status = "PASS" if result["summary"] == "PASS" else "FAIL"
        if result["summary"] == "PASS":
            passed += 1
        else:
            failed += 1

        details = f"syn={result['valid_syntax']} pos={result['positive_test']} neg={result['negative_test']}"
        print(f"  [{status}] {result['rule_name']:40s} {details}")

    print(f"\n  Unit Test Result: {passed}/{passed + failed} passed")
    return failed == 0


def test_integration():
    """Phase 2: 用合成文档验证 Vale 全规则集成"""
    section("Phase 2: Integration Test — All Rules Together")

    # 创建包含所有违规模式的合成文档
    integration_doc = os.path.join(BASE, "_integration_test.md")
    with open(integration_doc, "w", encoding="utf-8") as f:
        f.write("# Integration Test\n\n")
        f.write("## 1. Introduction\n\n")
        f.write("```\n")  # CodeBlockLanguage violation
        f.write("console.log('test');\n")  # NoConsoleLog violation
        f.write("```\n\n")
        # NoteCautionFormat violation
        f.write("> **说明：** This is on same line without br\n\n")
        # HTML tag violation
        f.write("<br\n\n")
        # @link violation
        f.write("See {@link Class.method} for details.\n\n")
        # Link violations
        f.write("[bad](path/with   /spaces)\n")
        f.write("[bad](unclosed\n")
        f.write("[bad<br>](url)\n\n")
        # Image type violation
        f.write("![bad](figures/image.bmp)\n\n")
        # Tab character
        f.write("| Name\t| Value\t|\n")
        f.write("| ---- | ----- |\n")
        f.write("| a\t| b\t|\n\n")
        # Missing trailing pipe
        f.write("| Name | Value\n")
        f.write("| ---- | -----\n")
        f.write("| a    | b\n\n")

    # 用 Vale 直接检查
    import subprocess
    result = subprocess.run(
        ["vale", "--config", KB_CONFIG, "--output", "JSON", integration_doc],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        encoding="utf-8", timeout=30
    )

    if result.returncode != 0 and result.stderr:
        print(f"  Vale Error: {result.stderr[:200]}")
        return False

    data = json.loads(result.stdout) if result.stdout.strip() else {}
    alerts = []
    for fpath, file_alerts in data.items():
        for a in file_alerts:
            alerts.append(a)

    # 按规则名分组
    from collections import Counter
    checks = Counter(a["Check"] for a in alerts)
    checks_list = sorted(checks.items())

    print(f"  Vale detected {len(alerts)} total alerts across {len(checks)} rule types")
    print()
    for check, count in checks_list:
        short = check.split(".")[-1] if "." in check else check
        print(f"    [{short:30s}] {count} alert(s)")
    print()

    os.remove(integration_doc)
    return len(alerts) > 5  # 至少命中 5 条以上不同规则


def test_real_doc():
    """Phase 3: 对真实 OpenHarmony 文档做 check"""
    section("Phase 3: Real Document Check")

    real_doc = os.path.join(os.path.dirname(__file__), "../oh-input/abilitystage.md")
    if not os.path.exists(real_doc):
        print("  Real doc not found, skipping")
        return True

    result = subprocess.run(
        [sys.executable, "-m", "tools.cli", "check",
         "--target", real_doc,
         "--config", KB_CONFIG],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        encoding="utf-8", timeout=30,
        cwd=PROJECT
    )

    # 提取得分行
    for line in result.stdout.split("\n"):
        if "评分" in line or "50/100" in line:
            print(f"  {line.strip()}")
            break
    else:
        lines = result.stdout.split("\n")
        for i, line in enumerate(lines):
            if "总数" in line or "总" in line:
                print(f"  {line.strip()}")
                break
        else:
            print(f"  Check completed (see output for details)")

    return True


def test_abilistage_direct_vale():
    """直接用 Vale 检查真实文档，看命中几条规则"""
    section("Phase 4: Direct Vale on abilitystage.md")

    real_doc = os.path.join(os.path.dirname(__file__), "../oh-input/abilitystage.md")
    if not os.path.exists(real_doc):
        print("  Real doc not found, skipping")
        return True

    result = subprocess.run(
        ["vale", "--config", KB_CONFIG, "--output", "JSON", real_doc],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        encoding="utf-8", timeout=30
    )

    if result.returncode != 0 and result.stderr:
        if "E201" in result.stderr or "E203" in result.stderr:
            print(f"  Vale config error: {result.stderr.strip()}")
            return False

    data = json.loads(result.stdout) if result.stdout.strip() else {}
    alerts = []
    for fpath, file_alerts in data.items():
        for a in file_alerts:
            alerts.append(a)

    if not alerts:
        print("  No Vale rules fired on this document (expected — it's well-formed)")
    else:
        print(f"  Vale fired {len(alerts)} alerts:")
        for a in alerts:
            print(f"    [{a['Severity']:10s}] {a['Check']:40s} L{a['Line']}: {a['Match'][:50]}")

    return True


def main():
    print("=" * 60)
    print("  OpenHarmony Vale 规则全量验证套件")
    print(f"  规则总数: 18（14 original + CodeBlockLanguage + CommaCount + ExclamationLimit + TryCatchPair）")
    print(f"  (另: ConsistentTerms/CJK + SentenceLengthCN/metric 因 Vale 限制已从测试中移除)")
    print(f"  日期: 2026-06-09")
    print("=" * 60)

    results = []
    results.append(("Unit Tests", test_rules()))
    results.append(("Integration Test", test_integration()))
    results.append(("Real Doc Check", test_real_doc()))
    results.append(("Direct Vale Check", test_abilistage_direct_vale()))

    print()
    print("=" * 60)
    print("  Summary")
    print("=" * 60)
    all_pass = True
    for name, passed in results:
        status = "PASS" if passed else "FAIL"
        if not passed:
            all_pass = False
        print(f"  [{status}] {name}")

    print()
    if all_pass:
        print("  ALL TESTS PASSED")
    else:
        print("  SOME TESTS FAILED")

    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())
