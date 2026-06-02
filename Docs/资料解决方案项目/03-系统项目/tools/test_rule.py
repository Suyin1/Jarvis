"""doc-solution test-rule — Vale rule validation tool

Tests a Vale rule YAML file for:
1. Syntax validity (Vale accepts the rule without error)
2. Positive test (rule catches intended violations)
3. Negative test (rule does NOT produce false positives)

This is a GENERIC test harness — it works with any Vale rule type
(existence, substitution, occurrence, repetition, consistency,
conditional, capitalization, spelling, metric) without knowing the
specific content the rule checks.

Usage:
    doc-solution test-rule --rule <rule.yml> [--should-pass <pass.md>] [--should-fail <fail.md>]
"""

import json
import shutil
import tempfile
from pathlib import Path

import click
import yaml

from engine.rule_engine.vale_adapter import ValeAdapter, ValeConfig


def run_test_rule(rule_file, should_pass=None, should_fail=None):
    """Validate a Vale rule against test documents.

    Args:
        rule_file: Path to a Vale rule YAML file.
        should_pass: Path to a .md file that should NOT trigger the rule.
        should_fail: Path to a .md file that SHOULD trigger the rule.

    Returns:
        dict with test results:
        {
            "rule_file": str,
            "rule_name": str,
            "valid_syntax": bool,
            "syntax_error": str or None,
            "positive_test": bool or None,   # None = skipped
            "negative_test": bool or None,   # None = skipped
            "alerts_on_fail": [...],
            "alerts_on_pass": [...],
            "summary": "PASS" | "FAIL" | "SYNTAX_ERROR" | "SKIPPED"
        }
    """
    rule_path = Path(rule_file)
    result = {
        "rule_file": str(rule_path),
        "rule_name": None,
        "valid_syntax": False,
        "syntax_error": None,
        "positive_test": None,
        "negative_test": None,
        "alerts_on_fail": [],
        "alerts_on_pass": [],
        "summary": "SKIPPED",
    }

    if not rule_path.exists():
        result["syntax_error"] = "Rule file not found: %s" % rule_file
        result["summary"] = "SYNTAX_ERROR"
        return result

    # Extract rule namespace from parent directory name
    # e.g., styles/Custom/TermSubstitution.yml -> namespace=Custom, name=TermSubstitution
    namespace = rule_path.parent.name
    rule_name = rule_path.stem
    result["rule_name"] = "%s.%s" % (namespace, rule_name)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        styles_dir = tmp / "styles" / namespace
        styles_dir.mkdir(parents=True)

        # Copy rule file into temp styles directory
        shutil.copy2(rule_path, styles_dir / rule_path.name)

        # Create .vale.ini that enables only this namespace
        ini_content = (
            "StylesPath = styles\n"
            "MinAlertLevel = suggestion\n"
            "\n"
            "[*.md]\n"
            "BasedOnStyles = %s\n" % namespace
        )
        (tmp / ".vale.ini").write_text(ini_content, encoding="utf-8")

        config = ValeConfig(config_path=str(tmp / ".vale.ini"))
        adapter = ValeAdapter(config)

        # --- Step 1: Syntax validation ---
        syntax_test = tmp / "_syntax_test.md"
        syntax_test.write_text(
            "# Syntax Validation\n\n"
            "Minimal test content for Vale rule syntax validation.\n",
            encoding="utf-8",
        )
        syntax_result = adapter.check(str(syntax_test))

        if syntax_result.exit_code == -1 and syntax_result.error_message:
            result["syntax_error"] = syntax_result.error_message
            result["summary"] = "SYNTAX_ERROR"
            return result

        # Check for Vale configuration errors
        if syntax_result.error_message:
            result["syntax_error"] = syntax_result.error_message
            result["summary"] = "SYNTAX_ERROR"
            return result

        result["valid_syntax"] = True

        # --- Step 2: Positive test (should-fail) ---
        if should_fail:
            fail_path = Path(should_fail)
            if not fail_path.exists():
                result["positive_test"] = False
                result["alerts_on_fail"] = [{"error": "should-fail file not found: %s" % should_fail}]
            else:
                fail_result = adapter.check(str(fail_path))
                _collect_alerts(result, "alerts_on_fail", fail_result)
                result["positive_test"] = _has_matching_alert(result, "alerts_on_fail")

        # --- Step 3: Negative test (should-pass) ---
        if should_pass:
            pass_path = Path(should_pass)
            if not pass_path.exists():
                result["negative_test"] = False
                result["alerts_on_pass"] = [{"error": "should-pass file not found: %s" % should_pass}]
            else:
                pass_result = adapter.check(str(pass_path))
                _collect_alerts(result, "alerts_on_pass", pass_result)
                result["negative_test"] = not _has_matching_alert(result, "alerts_on_pass")

    # --- Determine summary ---
    result["summary"] = "PASS"
    if should_fail and result["positive_test"] is False:
        result["summary"] = "FAIL"
    if should_pass and result["negative_test"] is False:
        result["summary"] = "FAIL"

    return result


def _collect_alerts(result, field, vale_result):
    """Collect all alerts from Vale result into the specified field."""
    for file_path, file_result in vale_result.files.items():
        for alert in file_result.alerts:
            result[field].append({
                "check": alert.check,
                "message": alert.message,
                "severity": alert.severity,
                "match": alert.match,
                "file": file_path,
                "line": alert.line,
            })


def _has_matching_alert(result, field):
    """Check if any alerts belong to the rule being tested."""
    rule_name = result["rule_name"]
    for alert in result[field]:
        if alert.get("check") == rule_name:
            return True
    return len(result[field]) > 0


def _format_result_text(result):
    """Format test result as human-readable text."""
    lines = []
    lines.append("")
    lines.append("Rule:    %s" % result["rule_name"])
    lines.append("File:    %s" % result["rule_file"])
    lines.append("Summary: %s" % result["summary"])
    lines.append("")

    # Syntax
    lines.append("[Syntax]")
    if result["valid_syntax"]:
        lines.append("  Valid:  YES")
    else:
        lines.append("  Valid:  NO")
        if result["syntax_error"]:
            lines.append("  Error: %s" % result["syntax_error"])
    lines.append("")

    # Positive test
    lines.append("[Positive Test (should-fail)]")
    if result["positive_test"] is None:
        lines.append("  Result: SKIPPED (no --should-fail provided)")
    elif result["positive_test"]:
        lines.append("  Result: PASS — rule caught violations")
    else:
        lines.append("  Result: FAIL — rule did NOT catch violations")
    if result["alerts_on_fail"]:
        for a in result["alerts_on_fail"]:
            lines.append("  Alert:  [%s] %s" % (a["severity"], a["message"]))
    lines.append("")

    # Negative test
    lines.append("[Negative Test (should-pass)]")
    if result["negative_test"] is None:
        lines.append("  Result: SKIPPED (no --should-pass provided)")
    elif result["negative_test"]:
        lines.append("  Result: PASS — no false positives")
    else:
        lines.append("  Result: FAIL — false positives detected")
    if result["alerts_on_pass"]:
        for a in result["alerts_on_pass"]:
            lines.append("  Alert:  [%s] %s" % (a["severity"], a["message"]))
    lines.append("")

    return "\n".join(lines)


@click.command(name="test-rule")
@click.option(
    "--rule", "-r",
    required=True,
    help="Path to Vale rule YAML file",
)
@click.option(
    "--should-pass",
    help="Path to .md file that should NOT trigger the rule (negative test)",
)
@click.option(
    "--should-fail",
    help="Path to .md file that SHOULD trigger the rule (positive test)",
)
@click.option(
    "--output", "-o",
    type=click.Choice(["text", "json"]),
    default="text",
    help="Output format",
)
def test_rule_command(rule, should_pass, should_fail, output):
    """Validate a Vale rule against positive and negative test documents."""
    result = run_test_rule(rule, should_pass, should_fail)

    if output == "json":
        click.echo(json.dumps(result, indent=2, ensure_ascii=False))
    else:
        click.echo(_format_result_text(result))

    if result["summary"] == "SYNTAX_ERROR":
        raise click.Abort()
    if result["summary"] == "FAIL":
        raise click.Abort()
