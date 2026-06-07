# 契约驱动流水线的核心数据模型

import json
from dataclasses import dataclass, field, asdict
from enum import Enum
from pathlib import Path
from typing import Optional, List, Dict, Any


# ---------------------------------------------------------------------------
# Enums
# ---------------------------------------------------------------------------

class Stage(Enum):
    SPEC_GENERATION = "spec_generation"
    CODE_GENERATION = "code_generation"
    INTEGRATION_BUILD = "integration_build"
    VERIFICATION = "verification"
    ARCHIVE = "archive"


class WorkerType(Enum):
    CPP = "cpp"
    NAPI = "napi"
    ARKTS = "arkts"
    CONFIG = "config"


class WorkerStatus(Enum):
    PENDING = "pending"
    RUNNING = "running"
    SUCCESS = "success"
    FAILED = "failed"
    SKIPPED = "skipped"


class TaskStatus(Enum):
    PENDING = "pending"
    IN_PROGRESS = "in_progress"
    PASSED = "passed"
    FAILED = "failed"
    BLOCKED = "blocked"


class Severity(Enum):
    INFO = "info"
    WARNING = "warning"
    ERROR = "error"
    CRITICAL = "critical"


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class InterfaceContract:
    """Interface contract defining cross-module APIs."""
    module_name: str
    cpp_header_declaration: str = ""
    napi_function_signatures: List[str] = field(default_factory=list)
    arkts_import_declaration: str = ""
    arkts_method_signatures: List[str] = field(default_factory=list)
    data_types: List[dict] = field(default_factory=list)

    def to_markdown(self) -> str:
        lines = [f"## Interface Contract: {self.module_name}", ""]
        if self.cpp_header_declaration:
            lines.extend(["### C++ Header", "```cpp", self.cpp_header_declaration, "```", ""])
        if self.napi_function_signatures:
            lines.extend(["### NAPI Signatures", "```cpp"] + self.napi_function_signatures + ["```", ""])
        if self.arkts_import_declaration:
            lines.extend(["### ArkTS Import", "```typescript", self.arkts_import_declaration, "```", ""])
        if self.arkts_method_signatures:
            lines.extend(["### ArkTS Methods", "```typescript"] + self.arkts_method_signatures + ["```", ""])
        return "\n".join(lines)


@dataclass
class TestCase:
    """Single test case specification."""
    id: str
    description: str
    steps: List[str] = field(default_factory=list)
    expected_result: str = ""
    preconditions: List[str] = field(default_factory=list)

    def to_markdown(self) -> str:
        lines = [
            f"### TC-{self.id}: {self.description}", "",
            "**Preconditions:**" if self.preconditions else "",
        ]
        if self.preconditions:
            lines.extend(f"- {p}" for p in self.preconditions)
            lines.append("")
        lines.extend(["**Steps:**"] + [f"{i+1}. {s}" for i, s in enumerate(self.steps)] + [""])
        lines.append(f"**Expected:** {self.expected_result}")
        return "\n".join(lines)


@dataclass
class BuildConfigChange:
    """Build configuration change specification."""
    file_path: str = ""
    change_type: str = "modify"  # add | modify | delete
    content: str = ""
    description: str = ""


@dataclass
class Specification:
    """Structured specification document (Stage 1 output)."""
    title: str
    version: str = "1.0"
    description: str = ""
    author: str = "pipeline"
    created_at: str = ""

    # Core content
    functional_description: str = ""
    interaction_flow: List[str] = field(default_factory=list)
    state_changes: List[dict] = field(default_factory=list)

    # Interface contracts
    interfaces: List[InterfaceContract] = field(default_factory=list)

    # File manifest
    files_to_create: List[str] = field(default_factory=list)
    files_to_modify: List[str] = field(default_factory=list)

    # Build changes
    build_config_changes: List[BuildConfigChange] = field(default_factory=list)

    # Test cases
    test_cases: List[TestCase] = field(default_factory=list)

    # Constraints
    constraints: List[str] = field(default_factory=list)
    dependencies: List[str] = field(default_factory=list)

    def to_markdown(self) -> str:
        lines = [
            f"# {self.title}",
            f"Version: {self.version} | Author: {self.author} | Created: {self.created_at}",
            "",
            "## Functional Description",
            self.functional_description, "",
            "## Interaction Flow",
        ]
        lines.extend(f"{i+1}. {s}" for i, s in enumerate(self.interaction_flow))
        lines.append("")
        lines.extend([
            "## State Changes",
        ])
        for sc in self.state_changes:
            lines.append(f"- **{sc.get('trigger', '?')}** → {sc.get('result', '?')}")
        lines.append("")

        for iface in self.interfaces:
            lines.append(iface.to_markdown())

        lines.extend(["", "## File Manifest", "### Create"])
        lines.extend(f"- `{f}`" for f in self.files_to_create)
        lines.extend(["", "### Modify"])
        lines.extend(f"- `{f}`" for f in self.files_to_modify)

        lines.extend(["", "## Build Changes"])
        for bc in self.build_config_changes:
            lines.append(f"- `{bc.file_path}` [{bc.change_type}]: {bc.description}")

        lines.extend(["", "## Test Cases"])
        for tc in self.test_cases:
            lines.append(tc.to_markdown())

        if self.constraints:
            lines.extend(["", "## Constraints"])
            lines.extend(f"- {c}" for c in self.constraints)

        return "\n".join(lines)

    def to_dict(self) -> dict:
        return asdict(self)

    def save(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(self.to_markdown())

    @classmethod
    def from_dict(cls, d: dict) -> "Specification":
        interfaces = [InterfaceContract(**i) if isinstance(i, dict) else i for i in d.get("interfaces", [])]
        test_cases = [TestCase(**t) if isinstance(t, dict) else t for t in d.get("test_cases", [])]
        build_config_changes = [BuildConfigChange(**b) if isinstance(b, dict) else b for b in d.get("build_config_changes", [])]
        spec = cls(
            title=d.get("title", "Untitled"),
            version=d.get("version", "1.0"),
            description=d.get("description", ""),
            author=d.get("author", "pipeline"),
            created_at=d.get("created_at", ""),
            functional_description=d.get("functional_description", ""),
            interaction_flow=d.get("interaction_flow", []),
            state_changes=d.get("state_changes", []),
            interfaces=interfaces,
            files_to_create=d.get("files_to_create", []),
            files_to_modify=d.get("files_to_modify", []),
            build_config_changes=build_config_changes,
            test_cases=test_cases,
            constraints=d.get("constraints", []),
            dependencies=d.get("dependencies", []),
        )
        return spec


@dataclass
class WorkerTask:
    """A task assigned to a code generation worker."""
    task_id: str
    worker_type: WorkerType
    specification: Specification
    knowledge_base_path: str = ""
    output_dir: str = ""
    status: WorkerStatus = WorkerStatus.PENDING
    result_files: List[str] = field(default_factory=list)
    error_message: str = ""
    attempt_count: int = 0


@dataclass
class RepairTask:
    """A repair task generated from verification failures."""
    task_id: str
    original_task_id: str
    worker_type: WorkerType
    error_description: str
    file_paths: List[str] = field(default_factory=list)
    error_log: str = ""
    fix_attempts: int = 0
    status: TaskStatus = TaskStatus.PENDING


@dataclass
class BuildResult:
    """Result of a build step."""
    success: bool
    stage: str = ""  # "qt" | "harmony"
    stdout: str = ""
    stderr: str = ""
    errors: List[dict] = field(default_factory=list)
    duration_seconds: float = 0.0


@dataclass
class VerificationResult:
    """Result of verification stage."""
    total_tests: int = 0
    passed: int = 0
    failed: int = 0
    skipped: int = 0
    details: List[dict] = field(default_factory=list)
    artifacts: List[str] = field(default_factory=list)

    @property
    def pass_rate(self) -> float:
        if self.total_tests == 0:
            return 0.0
        return self.passed / self.total_tests

    @property
    def all_passed(self) -> bool:
        return self.failed == 0 and self.total_tests > 0


@dataclass
class PipelineContext:
    """Shared context passed through pipeline stages."""
    requirement: str = ""
    spec_path: str = ""
    specification: Optional[Specification] = None
    worker_outputs: Dict[str, List[str]] = field(default_factory=lambda: {
        "cpp": [], "napi": [], "arkts": [], "config": []
    })
    build_results: List[BuildResult] = field(default_factory=list)
    verification_result: Optional[VerificationResult] = None
    repair_iterations: int = 0
    repair_tasks: List[RepairTask] = field(default_factory=list)
    status: TaskStatus = TaskStatus.PENDING
    pipeline_id: str = ""
    logs: List[dict] = field(default_factory=list)

    def add_log(self, stage: str, message: str, severity: Severity = Severity.INFO) -> None:
        import datetime
        self.logs.append({
            "timestamp": datetime.datetime.now().isoformat(),
            "stage": stage,
            "message": message,
            "severity": severity.value,
        })


@dataclass
class MemoryEntry:
    """A single entry in the pipeline memory/knowledge store."""
    entry_id: str
    title: str
    requirement: str
    spec_summary: str
    key_lessons: List[str] = field(default_factory=list)
    failure_patterns: List[dict] = field(default_factory=list)
    success_patterns: List[dict] = field(default_factory=list)
    file_paths: List[str] = field(default_factory=list)
    created_at: str = ""
    tags: List[str] = field(default_factory=list)
