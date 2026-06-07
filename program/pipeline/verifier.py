# Stage 4: 验证与自愈系统
# 运行测试、评估结果、管理修复闭环

import json
import logging
import subprocess
import time
from pathlib import Path
from typing import Optional, List, Dict

from .models import (
    VerificationResult, PipelineContext, WorkerTask, WorkerType,
    RepairTask, Severity, TaskStatus,
)
from .worker_pool import CodeWorker
from .knowledge_base import KnowledgeBase

logger = logging.getLogger(__name__)


class Verifier:
    """Stage 4 pipeline component.

    Runs automated verification (tests, linting, static analysis) and
    manages the self-healing repair loop by feeding failures back to
    the appropriate workers.
    """

    def __init__(self, config: dict, knowledge_base: KnowledgeBase):
        self.config = config
        self.knowledge_base = knowledge_base
        self.verification_config = config.get("pipeline", {}).get("verification", {})
        self.max_repair_iterations = config.get("pipeline", {}).get("max_repair_iterations", 5)
        self.required_pass_rate = self.verification_config.get("required_pass_rate", 1.0)

    # ------------------------------------------------------------------
    # Verification
    # ------------------------------------------------------------------

    def verify(self, context: Optional[PipelineContext] = None) -> VerificationResult:
        """Run all verification steps.

        Executes automated tests and returns results.
        """
        logger.info("Starting verification...")
        if context:
            context.add_log("verification", "Starting verification")

        result = VerificationResult()

        # Run test scripts
        test_script = self._find_test_script()
        if test_script:
            test_result = self._run_test_script(test_script)
            result = test_result
        else:
            # Run basic compilation check as minimal verification
            result = self._run_syntax_checks(context)

        if context:
            context.verification_result = result
            context.add_log("verification",
                            f"Tests: {result.passed}/{result.total_tests} passed "
                            f"({result.pass_rate*100:.0f}%)")

        logger.info(f"Verification complete: {result.passed}/{result.total_tests} passed")
        return result

    def _find_test_script(self) -> Optional[Path]:
        """Find the test script in the workspace or scripts directory."""
        candidates = [
            Path("scripts/test.sh"),
            Path("scripts/test.py"),
            Path("workspace/test.sh"),
        ]
        for c in candidates:
            if c.exists():
                return c
        return None

    def _run_test_script(self, script_path: Path) -> VerificationResult:
        """Execute a test script and parse results."""
        timeout = self.verification_config.get("test_timeout_seconds", 60)
        try:
            result = subprocess.run(
                [sys.executable if script_path.suffix == ".py" else "bash", str(script_path)],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout,
            )
            return self._parse_test_output(result.stdout, result.stderr)
        except subprocess.TimeoutExpired:
            logger.error(f"Test script timed out after {timeout}s")
            return VerificationResult(total_tests=1, failed=1, details=[
                {"error": "Test timeout", "message": f"Exceeded {timeout}s"}
            ])
        except Exception as e:
            logger.error(f"Test script execution failed: {e}")
            return VerificationResult(total_tests=1, failed=1, details=[
                {"error": "Test execution error", "message": str(e)}
            ])

    def _run_syntax_checks(self, context: Optional[PipelineContext] = None) -> VerificationResult:
        """Run basic syntax/compilation checks as minimal verification."""
        result = VerificationResult()
        checks = []

        # Check for .ets files with basic syntax patterns
        ets_files = list(Path("output/ets").rglob("*.ets"))
        for f in ets_files:
            content = f.read_text(encoding="utf-8")
            issues = []
            if "import" not in content and "export" not in content and "@Entry" not in content:
                issues.append("Missing import/export or @Entry decorator")
            if "@Component" not in content and "@Entry" not in content:
                issues.append("No @Component or @Entry decorator found")

            checks.append({
                "file": str(f),
                "issues": issues,
                "passed": len(issues) == 0,
            })

        result.details = checks
        result.total_tests = len(checks)
        result.passed = sum(1 for c in checks if c["passed"])
        result.failed = result.total_tests - result.passed

        return result

    def _parse_test_output(self, stdout: str, stderr: str) -> VerificationResult:
        """Parse test output to extract pass/fail counts.

        Supports standard TAP format and common test output patterns.
        """
        result = VerificationResult()
        lines = (stdout + "\n" + stderr).split("\n")

        for line in lines:
            line = line.strip()
            # TAP format: "ok 1 description" or "not ok 2 description"
            if line.startswith("ok "):
                result.passed += 1
                result.total_tests += 1
                result.details.append({"status": "pass", "message": line[3:]})
            elif line.startswith("not ok "):
                result.failed += 1
                result.total_tests += 1
                result.details.append({"status": "fail", "message": line[7:]})

        # If no TAP format found, check overall pass/fail
        if result.total_tests == 0:
            if "PASS" in stdout or "SUCCESS" in stdout or "0 failures" in stdout:
                result.total_tests = 1
                result.passed = 1
            elif "FAIL" in stdout or "ERROR" in stdout:
                result.total_tests = 1
                result.failed = 1

        return result

    # ------------------------------------------------------------------
    # Self-Healing
    # ------------------------------------------------------------------

    def needs_repair(self, result: VerificationResult, build_results: list) -> bool:
        """Determine if repair is needed based on verification results."""
        if result.failed > 0:
            return True
        if not build_results:
            return False
        return not all(br.success for br in build_results)

    def create_repair_tasks_from_verification(
        self, result: VerificationResult,
        context: Optional[PipelineContext] = None
    ) -> List[RepairTask]:
        """Generate repair tasks from verification failures."""
        repair_tasks = []

        for detail in result.details:
            if detail.get("status") != "fail" and not detail.get("issues"):
                continue

            file_path = detail.get("file", "")
            issues = detail.get("issues", [detail.get("message", "Unknown failure")])

            worker_type = self._map_file_to_worker(file_path)

            repair_tasks.append(RepairTask(
                task_id=f"vrepair-{len(repair_tasks)}",
                original_task_id=f"{worker_type.value}-0",
                worker_type=worker_type,
                error_description="; ".join(issues) if isinstance(issues, list) else issues,
                file_paths=[file_path] if file_path else [],
                error_log=json.dumps(detail, ensure_ascii=False),
            ))

        if context:
            context.add_log("verification",
                            f"Generated {len(repair_tasks)} repair tasks from verification failures",
                            Severity.WARNING if repair_tasks else Severity.INFO)

        return repair_tasks

    def _map_file_to_worker(self, file_path: str) -> WorkerType:
        """Map a file path to the responsible worker type."""
        path_lower = file_path.lower()
        if any(ext in path_lower for ext in [".cpp", ".h", ".hpp", ".cxx"]) and "napi" not in path_lower:
            return WorkerType.CPP
        if "napi" in path_lower or "bridge" in path_lower:
            return WorkerType.NAPI
        if any(ext in path_lower for ext in [".ets", ".ts"]):
            return WorkerType.ARKTS
        if any(ext in path_lower for ext in [".json5", ".json", "cmake"]):
            return WorkerType.CONFIG
        return WorkerType.CPP  # Default to C++

    def execute_repair(self, repair_task: RepairTask,
                       worker_outputs: Dict[str, List[str]],
                       context: Optional[PipelineContext] = None) -> Optional[WorkerTask]:
        """Execute a single repair task by mapping it to the appropriate worker.

        Args:
            repair_task: The repair task to execute.
            worker_outputs: Current worker outputs dict (updated in place).
            context: Optional pipeline context.

        Returns:
            The completed WorkerTask, or None if repair failed.
        """
        logger.info(f"Executing repair: {repair_task.task_id} -> {repair_task.worker_type.value}")

        # Create a code worker for the repair
        worker = CodeWorker(repair_task.worker_type, self.config, self.knowledge_base)

        # Build a repair-focused specification
        spec = None
        if context and context.specification:
            spec = context.specification
        else:
            from .models import Specification
            spec = Specification(title="Repair Task")

        # Create worker task with repair context
        worker_task = WorkerTask(
            task_id=repair_task.task_id,
            worker_type=repair_task.worker_type,
            specification=spec,
            output_dir=f"output/{repair_task.worker_type.value}",
            status=WorkerStatus.PENDING,
        )

        # Execute the repair
        result = worker.execute(worker_task, context)

        if result.status == WorkerStatus.SUCCESS:
            repair_task.status = TaskStatus.PASSED
            worker_outputs[repair_task.worker_type.value].extend(result.result_files)
            if context:
                context.add_log("repair",
                                f"Repair succeeded: {repair_task.task_id}",
                                Severity.INFO)
        else:
            repair_task.status = TaskStatus.FAILED
            fix_attempts = repair_task.fix_attempts + 1
            repair_task.fix_attempts = fix_attempts
            if context:
                context.add_log("repair",
                                f"Repair failed ({fix_attempts} attempts): {repair_task.task_id}",
                                Severity.WARNING)

        return result if result.status == WorkerStatus.SUCCESS else None

    def should_escalate(self, context: PipelineContext) -> bool:
        """Determine if the pipeline should escalate to human intervention."""
        return context.repair_iterations >= self.max_repair_iterations

    def generate_human_summary(self, context: PipelineContext) -> str:
        """Generate a summary for human intervention when auto-repair fails."""
        lines = [
            "# Pipeline Requires Human Intervention",
            "",
            f"**Requirement:** {context.requirement}",
            f"**Pipeline ID:** {context.pipeline_id}",
            f"**Repair Iterations:** {context.repair_iterations}",
            f"**Max Iterations:** {self.max_repair_iterations}",
            "",
            "## Build Results",
        ]

        for br in context.build_results:
            status = "PASS" if br.success else "FAIL"
            lines.append(f"- **{br.stage}**: {status} ({br.duration_seconds:.1f}s)")

        if context.verification_result:
            vr = context.verification_result
            lines.extend([
                "",
                "## Verification Results",
                f"- Total: {vr.total_tests}",
                f"- Passed: {vr.passed}",
                f"- Failed: {vr.failed}",
            ])

        lines.extend([
            "",
            "## Pending Repair Tasks",
        ])
        for rt in context.repair_tasks:
            if rt.status in (TaskStatus.PENDING, TaskStatus.FAILED):
                lines.append(f"- **{rt.task_id}** [{rt.worker_type.value}]: {rt.error_description}")

        lines.extend([
            "",
            "## Suggested Direction",
            "- Review the error logs above",
            "- Provide a hint or correction (e.g., 'Use rawdata mode for NAPI serialization')",
            "- The pipeline will resume with your guidance",
        ])

        return "\n".join(lines)
