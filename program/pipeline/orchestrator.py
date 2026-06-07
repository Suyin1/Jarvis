# 流水线主调度器 — 整个流水线的核心控制器
# 编排所有阶段：规约生成 → 代码生成 → 构建 → 验证 → 自愈 → 归档

import datetime
import json
import logging
import os
import sys
import time
import uuid
from pathlib import Path
from typing import Optional, List, Dict

import yaml

from .models import (
    Stage, TaskStatus, Severity, PipelineContext, BuildResult,
    Specification, WorkerTask, WorkerStatus, RepairTask,
)
from .spec_generator import SpecGenerator
from .worker_pool import WorkerPool
from .builder import Builder
from .verifier import Verifier
from .knowledge_base import KnowledgeBase
from .memory import PipelineMemory

logger = logging.getLogger(__name__)


class PipelineOrchestrator:
    """Main orchestrator that drives the contract-driven development pipeline.

    Flow:
        Requirement
          → Stage 1: SpecGenerator → Specification document
          → Stage 2: WorkerPool → Parallel code generation → Output files
          → Stage 3: Builder → Integration & build → BuildResult
          → Stage 4: Verifier → Verification → Repair loop
          → Archive + Memory update

    The pipeline self-heals by looping Stages 2-4 up to max_repair_iterations.
    """

    def __init__(self, config_path: str = "config/pipeline.yaml"):
        self.config = self._load_config(config_path)
        self._setup_logging()

        # Initialize subsystems
        self.knowledge_base = KnowledgeBase(
            self.config.get("pipeline", {}).get("knowledge_base", {}).get("path", "knowledge")
        )
        self.memory = PipelineMemory(
            self.config.get("pipeline", {}).get("memory", {}).get("path", "memory"),
            max_entries=self.config.get("pipeline", {}).get("memory", {}).get("max_entries", 100),
        )
        self.spec_generator = SpecGenerator(self.config, self.knowledge_base, self.memory)
        self.worker_pool = WorkerPool(self.config, self.knowledge_base)
        self.builder = Builder(self.config)
        self.verifier = Verifier(self.config, self.knowledge_base)

        # Pipeline state
        self._running = False
        self._abort = False

    def _load_config(self, config_path: str) -> dict:
        """Load YAML configuration file."""
        path = Path(config_path)
        if not path.exists():
            logger.warning(f"Config file not found: {config_path}, using defaults")
            return self._default_config()

        with open(path, "r", encoding="utf-8") as f:
            return yaml.safe_load(f)

    def _default_config(self) -> dict:
        return {
            "pipeline": {
                "name": "Default Pipeline",
                "version": "1.0.0",
                "max_repair_iterations": 5,
                "max_concurrent_workers": 4,
                "llm": {"backend": "mock", "model": "mock"},
                "build": {
                    "qt": {"build_tool": "cmake", "timeout_seconds": 300},
                    "harmony": {"build_tool": "hvigorw", "timeout_seconds": 300},
                },
                "paths": {
                    "workspace": "workspace",
                    "output": "output",
                    "specs": "specs",
                    "logs": "logs",
                },
            }
        }

    def _setup_logging(self) -> None:
        """Configure structured logging."""
        log_dir = Path(self.config.get("pipeline", {}).get("paths", {}).get("logs", "logs"))
        log_dir.mkdir(parents=True, exist_ok=True)

        log_file = log_dir / f"pipeline_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.log"

        formatter = logging.Formatter(
            "[%(asctime)s] %(levelname)-8s %(name)s:%(lineno)d - %(message)s"
        )

        # File handler
        fh = logging.FileHandler(str(log_file), encoding="utf-8")
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(formatter)

        # Console handler
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(logging.INFO)
        ch.setFormatter(formatter)

        root = logging.getLogger()
        root.setLevel(logging.DEBUG)
        root.addHandler(fh)
        root.addHandler(ch)

        logger.info(f"Pipeline logging initialized. Log file: {log_file}")

    # ------------------------------------------------------------------
    # Main Pipeline Execution
    # ------------------------------------------------------------------

    def run(self, requirement: str) -> PipelineContext:
        """Execute the full pipeline for a single requirement.

        Args:
            requirement: Natural language requirement description.

        Returns:
            PipelineContext with full execution state and results.

        Raises:
            RuntimeError: If pipeline encounters unrecoverable errors.
        """
        self._running = True
        self._abort = False

        context = PipelineContext(
            requirement=requirement,
            pipeline_id=f"pipe-{uuid.uuid4().hex[:12]}",
            status=TaskStatus.IN_PROGRESS,
        )

        logger.info("=" * 60)
        logger.info(f"Pipeline [{context.pipeline_id}] started")
        logger.info(f"Requirement: {requirement[:200]}")
        logger.info("=" * 60)

        try:
            # ---- Stage 1: Specification Generation ----
            spec = self._run_stage1(requirement, context)
            if self._abort:
                return context

            context.specification = spec

            # ---- Stage 2-4 Self-Healing Loop ----
            repair_iteration = 0
            while repair_iteration <= self.config.get("pipeline", {}).get("max_repair_iterations", 5):
                context.repair_iterations = repair_iteration
                logger.info(f"\n--- Repair iteration {repair_iteration} ---")

                # ---- Stage 2: Parallel Code Generation ----
                worker_tasks = self._run_stage2(spec, context)
                if self._abort:
                    return context
                context.worker_tasks = worker_tasks

                # ---- Stage 3: Integration & Build ----
                build_results = self._run_stage3(context)
                if self._abort:
                    return context

                # Check if we need to do verification or just build succeeded
                all_builds_passed = all(br.success for br in build_results)
                if not all_builds_passed:
                    repair_tasks = self.builder.generate_repair_tasks(
                        build_results, worker_tasks, context
                    )
                    context.repair_tasks = repair_tasks
                    if self._handle_repair(repair_tasks, context):
                        repair_iteration += 1
                        continue
                    else:
                        break

                # ---- Stage 4: Verification ----
                verification_result = self._run_stage4(context)

                if verification_result.all_passed:
                    logger.info("All verification checks passed!")
                    context.status = TaskStatus.PASSED
                    break

                # Generate repair tasks from verification
                repair_tasks = self.verifier.create_repair_tasks_from_verification(
                    verification_result, context
                )
                context.repair_tasks = repair_tasks

                if self._handle_repair(repair_tasks, context):
                    repair_iteration += 1
                    continue
                else:
                    break

            # ---- Final Status ----
            if context.status != TaskStatus.PASSED:
                if self.verifier.should_escalate(context):
                    context.status = TaskStatus.BLOCKED
                    summary = self.verifier.generate_human_summary(context)
                    logger.warning(f"Pipeline blocked — requires human intervention:\n{summary}")
                    # Save escalation report
                    self._save_escalation_report(context, summary)
                else:
                    context.status = TaskStatus.FAED

            # ---- Archive & Memory Update ----
            if context.status == TaskStatus.PASSED:
                self._run_archive(context)

        except Exception as e:
            logger.exception(f"Pipeline failed with exception: {e}")
            context.status = TaskStatus.FAILED
            context.add_log("orchestrator", f"Fatal error: {e}", Severity.CRITICAL)

        finally:
            self._running = False

        self._log_pipeline_summary(context)
        return context

    # ------------------------------------------------------------------
    # Stage Runners
    # ------------------------------------------------------------------

    def _run_stage1(self, requirement: str, context: PipelineContext) -> Specification:
        """Stage 1: Generate specification from requirement."""
        logger.info("\n" + "-" * 40)
        logger.info("STAGE 1: Specification Generation")
        logger.info("-" * 40)

        spec = self.spec_generator.generate(requirement, context)

        # Save spec to file
        specs_dir = Path(self.config.get("pipeline", {}).get("paths", {}).get("specs", "specs"))
        spec_path = specs_dir / f"{context.pipeline_id}.md"
        spec.save(spec_path)
        context.spec_path = str(spec_path)

        logger.info(f"Specification saved to: {spec_path}")
        logger.info(f"  Interfaces: {len(spec.interfaces)}")
        logger.info(f"  Files to create: {len(spec.files_to_create)}")
        logger.info(f"  Test cases: {len(spec.test_cases)}")

        return spec

    def _run_stage2(self, spec: Specification, context: PipelineContext) -> List[WorkerTask]:
        """Stage 2: Parallel code generation."""
        logger.info("\n" + "-" * 40)
        logger.info("STAGE 2: Parallel Code Generation")
        logger.info("-" * 40)

        tasks = self.worker_pool.create_tasks(spec, context)
        if not tasks:
            logger.warning("No worker tasks created — check specification content")
            return []

        completed = self.worker_pool.execute_all(tasks, context)
        return completed

    def _run_stage3(self, context: PipelineContext) -> List[BuildResult]:
        """Stage 3: Integration and build."""
        logger.info("\n" + "-" * 40)
        logger.info("STAGE 3: Integration & Build")
        logger.info("-" * 40)

        # Integrate code into workspace
        files_count = self.builder.integrate(context.worker_outputs, context)
        logger.info(f"Integrated {files_count} files into workspace")

        # Run builds
        build_results = self.builder.build_all(context)

        for br in build_results:
            status = "PASS" if br.success else "FAIL"
            logger.info(f"  [{br.stage}] Build {status} ({br.duration_seconds:.1f}s)")
            if not br.success:
                logger.info(f"  Errors: {len(br.errors)} classified")

        return build_results

    def _run_stage4(self, context: PipelineContext):
        """Stage 4: Verification."""
        logger.info("\n" + "-" * 40)
        logger.info("STAGE 4: Verification")
        logger.info("-" * 40)

        result = self.verifier.verify(context)
        logger.info(f"  Tests: {result.passed}/{result.total_tests} passed")
        return result

    # ------------------------------------------------------------------
    # Repair Loop
    # ------------------------------------------------------------------

    def _handle_repair(self, repair_tasks: List[RepairTask],
                       context: PipelineContext) -> bool:
        """Handle repair tasks. Returns True if repairs were attempted."""
        if not repair_tasks:
            return False

        logger.info(f"\n  --- Repair ({len(repair_tasks)} tasks) ---")
        all_succeeded = True

        for repair_task in repair_tasks:
            logger.info(f"  Repair task: {repair_task.task_id} "
                        f"[{repair_task.worker_type.value}]: "
                        f"{repair_task.error_description[:80]}")

            result = self.verifier.execute_repair(repair_task, context.worker_outputs, context)
            if result is None:
                all_succeeded = False
                repair_task.status = TaskStatus.FAILED

        return all_succeeded

    # ------------------------------------------------------------------
    # Archive & Memory
    # ------------------------------------------------------------------

    def _run_archive(self, context: PipelineContext) -> None:
        """Archive successful run and update memory."""
        logger.info("\n" + "-" * 40)
        logger.info("ARCHIVE & MEMORY UPDATE")
        logger.info("-" * 40)

        spec = context.specification
        if not spec:
            return

        # Extract lessons from logs
        failures = []
        successes = []
        for log_entry in context.logs:
            if log_entry.get("severity") in ("error", "critical"):
                failures.append({
                    "stage": log_entry.get("stage", ""),
                    "message": log_entry.get("message", ""),
                    "lesson": f"Avoid in {log_entry.get('stage', '')}: {log_entry.get('message', '')[:200]}",
                })
            elif log_entry.get("severity") == "info" and "succeeded" in log_entry.get("message", ""):
                successes.append({
                    "stage": log_entry.get("stage", ""),
                    "message": log_entry.get("message", ""),
                })

        # Create memory entry
        entry = self.memory.extract_lessons(
            requirement=context.requirement,
            spec_summary=spec.functional_description[:500],
            failures=failures,
            successes=successes,
        )

        # Collect file paths
        for wtype, files in context.worker_outputs.items():
            entry.file_paths.extend(files)

        self.memory.add_entry(entry)
        logger.info(f"Memory entry created: {entry.entry_id}")

        # Archive spec and key outputs
        archive_dir = Path(
            self.config.get("pipeline", {}).get("paths", {}).get("archive", "archive")
        ) / context.pipeline_id
        archive_dir.mkdir(parents=True, exist_ok=True)

        # Copy spec
        if context.spec_path:
            spec_file = Path(context.spec_path)
            if spec_file.exists():
                import shutil
                shutil.copy2(str(spec_file), str(archive_dir / "specification.md"))

        logger.info(f"Run archived to: {archive_dir}")

    # ------------------------------------------------------------------
    # Utilities
    # ------------------------------------------------------------------

    def _save_escalation_report(self, context: PipelineContext, summary: str) -> None:
        """Save a human-readable escalation report."""
        reports_dir = Path("reports")
        reports_dir.mkdir(exist_ok=True)
        report_path = reports_dir / f"escalation_{context.pipeline_id}.md"
        report_path.write_text(summary, encoding="utf-8")
        logger.info(f"Escalation report saved: {report_path}")

    def _log_pipeline_summary(self, context: PipelineContext) -> None:
        """Log a summary of the pipeline run."""
        status_emoji = {
            TaskStatus.PASSED: "PASS",
            TaskStatus.FAILED: "FAIL",
            TaskStatus.BLOCKED: "BLOCK",
            TaskStatus.IN_PROGRESS: "INPROG",
        }.get(context.status, "UNKNOWN")

        logger.info("\n" + "=" * 60)
        logger.info(f"PIPELINE COMPLETE [{status_emoji}]")
        logger.info(f"  Pipeline ID: {context.pipeline_id}")
        logger.info(f"  Status: {context.status.value}")
        logger.info(f"  Repair iterations: {context.repair_iterations}")
        logger.info(f"  Total log entries: {len(context.logs)}")

        for br in context.build_results:
            logger.info(f"  Build [{br.stage}]: {'PASS' if br.success else 'FAIL'}")

        if context.verification_result:
            vr = context.verification_result
            logger.info(f"  Tests: {vr.passed}/{vr.total_tests} passed")

        logger.info("=" * 60)

    def abort(self) -> None:
        """Request abort of the current pipeline run."""
        self._abort = True
        logger.warning("Pipeline abort requested — will stop after current stage")
