"""检查报告生成器测试"""

import json
from engine.checker.reporter import CheckReport, CheckResult, ReportItem, TraceStep


def test_empty_report():
    result = CheckResult()
    report = CheckReport(
        check_id="test-001",
        check_type="all",
        target="./docs/",
        result=result,
    )
    assert report.summary["total"] == 0
    assert report.summary["score"] == 100.0


def test_report_with_items():
    result = CheckResult()
    result.items.append(ReportItem(
        rule_id="heading-level",
        rule_name="标题层级",
        severity="error",
        status="failed",
        message="缺少一级标题",
        line=1,
    ))
    result.items.append(ReportItem(
        rule_id="para-length",
        rule_name="段落长度",
        severity="warning",
        status="failed",
        message="段落过长",
        line=10,
    ))

    report = CheckReport(
        check_id="test-002",
        check_type="structure",
        target="./test.md",
        result=result,
    )

    s = report.summary
    assert s["total"] == 2
    assert s["failed"] == 1
    assert s["warnings"] == 1
    assert s["score"] == 75.0  # 100 - 1*20 - 1*5


def test_report_to_dict():
    result = CheckResult()
    result.items.append(ReportItem(
        rule_id="test-rule",
        rule_name="测试规则",
        severity="error",
        status="failed",
        message="测试消息",
    ))

    report = CheckReport(
        check_id="test-003",
        check_type="format",
        target="./test.md",
        result=result,
    )

    d = report.to_dict()
    assert d["metadata"]["check_id"] == "test-003"
    assert d["summary"]["total"] == 1
    assert len(d["details"]) == 1


def test_report_to_json():
    result = CheckResult()
    result.items.append(ReportItem(
        rule_id="test-rule",
        rule_name="测试规则",
        severity="error",
        status="failed",
        message="测试消息",
    ))

    report = CheckReport(
        check_id="test-004",
        check_type="all",
        target="./test.md",
        result=result,
    )

    json_str = report.to_json()
    parsed = json.loads(json_str)
    assert parsed["metadata"]["check_id"] == "test-004"
    assert parsed["summary"]["total"] == 1


def test_merge_results():
    r1 = CheckResult()
    r1.items.append(ReportItem(
        rule_id="r1", rule_name="R1",
        severity="error", status="failed", message="msg1",
    ))

    r2 = CheckResult()
    r2.items.append(ReportItem(
        rule_id="r2", rule_name="R2",
        severity="warning", status="failed", message="msg2",
    ))

    r1.merge(r2)
    assert len(r1.items) == 2
    assert len(r1.errors) == 1
    assert len(r1.warnings) == 1


def test_trace_steps():
    result = CheckResult()
    result.trace.append(TraceStep(
        step="结构检查", tool="md_parser",
        status="completed", duration_ms=100,
    ))

    report = CheckReport(
        check_id="test-005",
        check_type="all",
        target="./",
        result=result,
    )

    d = report.to_dict()
    assert len(d["trace"]) == 1
    assert d["trace"][0]["step"] == "结构检查"
