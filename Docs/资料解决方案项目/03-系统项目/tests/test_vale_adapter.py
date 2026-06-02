"""Vale 适配器测试"""

from engine.rule_engine.vale_adapter import ValeAdapter, ValeConfig, ValeResult, ValeAlert, ValeFileResult


def test_vale_result_properties():
    file_result = ValeFileResult(file_path="test.md")
    file_result.alerts.append(ValeAlert(
        line=1, column=1, severity="error",
        message="错误", check="Test.Check", match="x",
    ))
    file_result.alerts.append(ValeAlert(
        line=2, column=1, severity="warning",
        message="警告", check="Test.Check", match="y",
    ))

    result = ValeResult()
    result.files["test.md"] = file_result

    assert result.total_errors == 1
    assert result.total_warnings == 1
    assert result.total_suggestions == 0
    assert result.total_alerts == 2
    assert not result.passed


def test_vale_result_passed():
    file_result = ValeFileResult(file_path="test.md")
    result = ValeResult()
    result.files["test.md"] = file_result
    assert result.passed


def test_vale_adapter_no_vale():
    """测试 Vale 不可用时的优雅降级"""
    adapter = ValeAdapter(ValeConfig(vale_bin="vale-not-exist"))
    result = adapter.check("./")
    assert result.exit_code == -1
    assert "未找到" in result.error_message


def test_vale_adapter_nonexistent_target():
    adapter = ValeAdapter()
    result = adapter.check("/nonexistent/path/12345")
    assert result.exit_code == -1
    assert "不存在" in result.error_message


def test_file_result_counts():
    f = ValeFileResult(file_path="test.md")
    assert f.error_count == 0
    assert f.warning_count == 0
    assert f.suggestion_count == 0

    f.alerts.append(ValeAlert(line=1, column=1, severity="error", message="e", check="C"))
    assert f.error_count == 1
    f.alerts.append(ValeAlert(line=2, column=1, severity="warning", message="w", check="C"))
    assert f.warning_count == 1
    f.alerts.append(ValeAlert(line=3, column=1, severity="suggestion", message="s", check="C"))
    assert f.suggestion_count == 1
