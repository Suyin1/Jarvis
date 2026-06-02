"""Vale 规则引擎适配器

封装 Vale 命令行工具的调用，将 Vale 输出转换为统一的检查报告格式。
Vale 是文档 Lint 工具，无需网络，本地运行。
"""

import json
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional


@dataclass
class ValeConfig:
    vale_bin: str = "vale"
    config_path: Optional[str] = None
    styles_path: Optional[str] = None


@dataclass
class ValeAlert:
    line: int
    column: int
    severity: str
    message: str
    check: str
    match: str = ""


@dataclass
class ValeFileResult:
    file_path: str
    alerts: List[ValeAlert] = field(default_factory=list)

    @property
    def error_count(self) -> int:
        return sum(1 for a in self.alerts if a.severity == "error")

    @property
    def warning_count(self) -> int:
        return sum(1 for a in self.alerts if a.severity == "warning")

    @property
    def suggestion_count(self) -> int:
        return sum(1 for a in self.alerts if a.severity == "suggestion")


@dataclass
class ValeResult:
    files: Dict[str, ValeFileResult] = field(default_factory=dict)
    raw_output: str = ""
    exit_code: int = 0
    error_message: str = ""

    @property
    def total_errors(self) -> int:
        return sum(f.error_count for f in self.files.values())

    @property
    def total_warnings(self) -> int:
        return sum(f.warning_count for f in self.files.values())

    @property
    def total_suggestions(self) -> int:
        return sum(f.suggestion_count for f in self.files.values())

    @property
    def total_alerts(self) -> int:
        return self.total_errors + self.total_warnings + self.total_suggestions

    @property
    def passed(self) -> bool:
        return self.exit_code == 0 and self.total_errors == 0


class ValeAdapter:
    """Vale 适配器，封装 Vale 命令行调用"""

    def __init__(self, config: Optional[ValeConfig] = None):
        self.config = config or ValeConfig()

    def check(self, target_path: str) -> ValeResult:
        """对目标文件或目录执行 Vale 检查"""
        target = Path(target_path)
        if not target.exists():
            return ValeResult(
                exit_code=-1,
                error_message=f"目标路径不存在: {target_path}"
            )

        cmd = [self.config.vale_bin, "--output", "JSON"]
        if self.config.config_path:
            cmd.extend(["--config", self.config.config_path])

        if target.is_file():
            cmd.append(str(target))
        else:
            cmd.append(str(target))

        try:
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
                timeout=120,
            )
        except FileNotFoundError:
            return ValeResult(
                exit_code=-1,
                error_message=(
                    f"Vale 未找到，请先安装 Vale。\n"
                    f"尝试设置 vale_bin 路径: {self.config.vale_bin}\n"
                    f"或确保 vale 在 PATH 中"
                )
            )
        except subprocess.TimeoutExpired:
            return ValeResult(
                exit_code=-1,
                error_message="Vale 检查超时（120秒）"
            )

        vale_result = ValeResult(
            raw_output=result.stdout,
            exit_code=result.returncode,
            error_message=result.stderr if result.returncode != 0 else "",
        )

        if result.stdout:
            vale_result = self._parse_json_output(result.stdout, vale_result)

        return vale_result

    def _parse_json_output(self, stdout: str, result: ValeResult) -> ValeResult:
        """解析 Vale 的 JSON 输出"""
        try:
            data = json.loads(stdout)
        except json.JSONDecodeError:
            return result

        for file_path, alerts in data.items():
            file_result = ValeFileResult(file_path=file_path)
            for alert in alerts:
                file_result.alerts.append(ValeAlert(
                    line=alert.get("Line", 0),
                    column=alert.get("Col", 0),
                    severity=alert.get("Severity", "suggestion").lower(),
                    message=alert.get("Message", ""),
                    check=alert.get("Check", ""),
                    match=alert.get("Match", ""),
                ))
            result.files[file_path] = file_result

        return result

    def list_rules(self) -> List[dict]:
        """列出当前配置的所有激活规则"""
        cmd = [self.config.vale_bin, "--output", "JSON", "--list"]
        if self.config.config_path:
            cmd.extend(["--config", self.config.config_path])

        try:
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=30)
            if result.returncode == 0 and result.stdout:
                return json.loads(result.stdout)
        except (FileNotFoundError, subprocess.TimeoutExpired, json.JSONDecodeError):
            pass

        return []
