"""检查报告生成器

将各类检查器的结果整合为统一的结构化报告，
支持 JSON/YAML 输出，符合 check-report.schema.json 格式。
"""

import json
from dataclasses import dataclass, field, asdict
from datetime import datetime, timezone
from typing import List, Optional


@dataclass
class ReportItem:
    rule_id: str
    rule_name: str
    severity: str  # error / warning / suggestion / needs_review
    status: str  # passed / failed / needs_review
    message: str
    file: str = ""
    line: int = 0
    column: int = 0
    section: str = ""
    expected: str = ""
    actual: str = ""
    suggestion: str = ""
    evidence: str = ""


@dataclass
class TraceStep:
    step: str
    tool: str
    status: str
    duration_ms: int = 0


@dataclass
class CheckResult:
    items: List[ReportItem] = field(default_factory=list)
    trace: List[TraceStep] = field(default_factory=list)

    @property
    def errors(self) -> List[ReportItem]:
        return [i for i in self.items if i.severity == "error"]

    @property
    def warnings(self) -> List[ReportItem]:
        return [i for i in self.items if i.severity == "warning"]

    @property
    def suggestions(self) -> List[ReportItem]:
        return [i for i in self.items if i.severity == "suggestion"]

    @property
    def needs_review(self) -> List[ReportItem]:
        return [i for i in self.items if i.severity == "needs_review"]

    def merge(self, other: "CheckResult") -> "CheckResult":
        self.items.extend(other.items)
        self.trace.extend(other.trace)
        return self


@dataclass
class CheckReport:
    check_id: str
    check_type: str
    target: str
    knowledge_version: str = ""
    result: CheckResult = field(default_factory=CheckResult)
    timestamp: str = ""

    def __post_init__(self):
        if not self.timestamp:
            self.timestamp = datetime.now(timezone.utc).isoformat()

    @property
    def summary(self) -> dict:
        return {
            "total": len(self.result.items),
            "passed": sum(1 for i in self.result.items if i.status == "passed"),
            "failed": len(self.result.errors),
            "warnings": len(self.result.warnings),
            "suggestions": len(self.result.suggestions),
            "needs_review": len(self.result.needs_review),
            "score": self._calculate_score(),
        }

    def _calculate_score(self) -> float:
        total = len(self.result.items)
        if total == 0:
            return 100.0
        errors = len(self.result.errors)
        warnings = len(self.result.warnings)
        score = max(0, 100 - errors * 20 - warnings * 5)
        return round(score, 1)

    def to_dict(self) -> dict:
        return {
            "metadata": {
                "check_id": self.check_id,
                "timestamp": self.timestamp,
                "check_type": self.check_type,
                "target": self.target,
                "knowledge_version": self.knowledge_version,
            },
            "summary": self.summary,
            "details": [asdict(item) for item in self.result.items],
            "trace": [asdict(step) for step in self.result.trace],
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    def to_text(self) -> str:
        """生成人类可读的文本报告"""
        lines = []
        lines.append("=" * 60)
        lines.append(f"检查报告: {self.check_id}")
        lines.append(f"类型: {self.check_type}  目标: {self.target}")
        lines.append(f"时间: {self.timestamp}")
        lines.append("-" * 60)

        s = self.summary
        lines.append(
            f"总计: {s['total']} | "
            f"通过: {s['passed']} | "
            f"失败: {s['failed']} | "
            f"警告: {s['warnings']} | "
            f"建议: {s['suggestions']} | "
            f"需确认: {s['needs_review']} | "
            f"评分: {s['score']}/100"
        )
        lines.append("-" * 60)

        for item in self.result.items:
            if item.status == "passed":
                continue
            icon = {"error": "X", "warning": "!", "suggestion": "?"}.get(
                item.severity, "?"
            )
            loc = "%s:%s" % (item.file, item.line) if item.file else "line %s" % item.line
            lines.append("  [%s] [%s] %s" % (icon, item.severity, item.message))
            lines.append("         location: %s" % loc)
            lines.append("         rule: %s (%s)" % (item.rule_name, item.rule_id))
            if item.suggestion:
                lines.append("         suggestion: %s" % item.suggestion)
            lines.append("")

        lines.append("=" * 60)
        return "\n".join(lines)
