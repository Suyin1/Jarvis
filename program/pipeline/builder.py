# Stage 3: 集成与构建系统
# 合并生成的代码，运行确定性构建脚本

import logging
import os
import shutil
import subprocess
import time
import re
from pathlib import Path
from typing import Optional, List, Dict

from .models import (
    BuildResult, PipelineContext, WorkerType, WorkerTask,
    RepairTask, Severity,
)

logger = logging.getLogger(__name__)


class Builder:
    """Stage 3 pipeline component.

    Merges code from worker outputs into the workspace, then runs
    deterministic build scripts. This stage is intentionally script-driven
    and does NOT use LLM calls to avoid hallucination.
    """

    def __init__(self, config: dict):
        self.config = config
        self.build_config = config.get("pipeline", {}).get("build", {})
        self.workspace = Path(config.get("pipeline", {}).get("paths", {}).get("workspace", "workspace"))

    # ------------------------------------------------------------------
    # Code Integration
    # ------------------------------------------------------------------

    def integrate(self, worker_outputs: Dict[str, List[str]],
                  context: Optional[PipelineContext] = None) -> int:
        """Merge generated code files into the workspace.

        Args:
            worker_outputs: Dict mapping worker type to list of file paths.
            context: Optional pipeline context.

        Returns:
            Number of files integrated.
        """
        workspace_src = self.workspace / "src"
        workspace_src.mkdir(parents=True, exist_ok=True)

        count = 0
        for worker_type, files in worker_outputs.items():
            target_dir_map = {
                "cpp": workspace_src / "cpp",
                "napi": workspace_src / "napi",
                "arkts": workspace_src / "ets",
                "config": workspace_src / "config",
            }
            target_dir = target_dir_map.get(worker_type, workspace_src)
            target_dir.mkdir(parents=True, exist_ok=True)

            for file_path_str in files:
                src = Path(file_path_str)
                if not src.exists():
                    logger.warning(f"File not found for integration: {src}")
                    continue
                dst = target_dir / src.name
                shutil.copy2(str(src), str(dst))
                count += 1
                logger.info(f"  Integrated: {src.name} -> {dst}")

        if context:
            context.add_log("integration", f"Integrated {count} files into workspace")

        logger.info(f"Integration complete: {count} files copied")
        return count

    # ------------------------------------------------------------------
    # Build Execution
    # ------------------------------------------------------------------

    def build_all(self, context: Optional[PipelineContext] = None) -> List[BuildResult]:
        """Run all build steps (QT and HarmonyOS).

        Returns:
            List of BuildResult objects for each build step.
        """
        results = []
        qt_config = self.build_config.get("qt", {})
        harmony_config = self.build_config.get("harmony", {})

        if qt_config.get("build_tool"):
            result = self._build_qt(qt_config, context)
            results.append(result)

        if harmony_config.get("build_tool"):
            result = self._build_harmony(harmony_config, context)
            results.append(result)

        if context:
            context.build_results = results

        return results

    def _build_qt(self, config: dict, context: Optional[PipelineContext] = None) -> BuildResult:
        """Run the QT build process."""
        build_dir = Path(config.get("build_dir", str(self.workspace / "build" / "qt")))
        build_dir.mkdir(parents=True, exist_ok=True)

        logger.info("Starting QT build...")
        if context:
            context.add_log("build", "Starting QT build")

        start = time.time()
        try:
            result = subprocess.run(
                ["cmake", "--build", str(build_dir), "--config", config.get("config", "Release")],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=config.get("timeout_seconds", 300),
                cwd=str(self.workspace),
            )
            elapsed = time.time() - start
            success = result.returncode == 0

            build_result = BuildResult(
                success=success,
                stage="qt",
                stdout=result.stdout,
                stderr=result.stderr,
                duration_seconds=elapsed,
            )

            if not success:
                build_result.errors = self._classify_errors(result.stderr, "qt")
                logger.error(f"QT build failed ({elapsed:.1f}s)")
                if context:
                    context.add_log("build", f"QT build failed ({elapsed:.1f}s)", Severity.ERROR)
            else:
                logger.info(f"QT build succeeded ({elapsed:.1f}s)")
                if context:
                    context.add_log("build", f"QT build succeeded ({elapsed:.1f}s)")

            return build_result

        except subprocess.TimeoutExpired:
            elapsed = time.time() - start
            logger.error(f"QT build timed out after {elapsed:.1f}s")
            if context:
                context.add_log("build", f"QT build timed out", Severity.ERROR)
            return BuildResult(success=False, stage="qt", stderr="Build timed out",
                               duration_seconds=elapsed)

    def _build_harmony(self, config: dict,
                       context: Optional[PipelineContext] = None) -> BuildResult:
        """Run the HarmonyOS build process."""
        build_dir = Path(config.get("build_dir", str(self.workspace / "build" / "harmony")))
        build_dir.mkdir(parents=True, exist_ok=True)

        logger.info("Starting HarmonyOS build...")
        if context:
            context.add_log("build", "Starting HarmonyOS build")

        start = time.time()
        try:
            result = subprocess.run(
                [config.get("build_tool", "hvigorw"), "assemble", config.get("config", "release")],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=config.get("timeout_seconds", 300),
                cwd=str(self.workspace),
            )
            elapsed = time.time() - start
            success = result.returncode == 0

            build_result = BuildResult(
                success=success,
                stage="harmony",
                stdout=result.stdout,
                stderr=result.stderr,
                duration_seconds=elapsed,
            )

            if not success:
                build_result.errors = self._classify_errors(result.stderr, "harmony")
                logger.error(f"HarmonyOS build failed ({elapsed:.1f}s)")
                if context:
                    context.add_log("build", f"HarmonyOS build failed ({elapsed:.1f}s)", Severity.ERROR)
            else:
                logger.info(f"HarmonyOS build succeeded ({elapsed:.1f}s)")
                if context:
                    context.add_log("build", f"HarmonyOS build succeeded ({elapsed:.1f}s)")

            return build_result

        except subprocess.TimeoutExpired:
            elapsed = time.time() - start
            logger.error(f"HarmonyOS build timed out after {elapsed:.1f}s")
            if context:
                context.add_log("build", f"HarmonyOS build timed out", Severity.ERROR)
            return BuildResult(success=False, stage="harmony",
                               stderr="Build timed out", duration_seconds=elapsed)

    # ------------------------------------------------------------------
    # Error Classification
    # ------------------------------------------------------------------

    def _classify_errors(self, stderr: str, stage: str) -> List[dict]:
        """Classify build errors by module type for targeted repair.

        This is the key function that enables self-healing by mapping
        errors to the responsible worker type.
        """
        errors = []
        patterns = {
            "cpp_compile": {
                "pattern": r'([\w/\\]+\.(?:cpp|h)):(\d+):(?:\d+:)?\s*(error|warning)',
                "worker_type": "cpp",
            },
            "napi_error": {
                "pattern": r'(?:napi|napi_env|napi_value|NAPI)',
                "worker_type": "napi",
            },
            "arkts_error": {
                "pattern": r'([\w/\\]+\.ets):(\d+)',
                "worker_type": "arkts",
            },
            "cmake_error": {
                "pattern": r'CMake\s+(Error|Warning)',
                "worker_type": "config",
            },
            "linker_error": {
                "pattern": r'(undefined reference|LNK\d+|ld:)',
                "worker_type": "cpp",
            },
        }

        for error_type, pattern_info in patterns.items():
            matches = re.finditer(pattern_info["pattern"], stderr, re.IGNORECASE)
            for m in matches:
                errors.append({
                    "type": error_type,
                    "worker_type": pattern_info["worker_type"],
                    "match": m.group(0),
                    "line": m.group(2) if len(m.groups()) >= 2 else "?",
                    "file": m.group(1) if len(m.groups()) >= 1 else "?",
                })

        return errors

    # ------------------------------------------------------------------
    # Repair Task Generation
    # ------------------------------------------------------------------

    def generate_repair_tasks(self, build_results: List[BuildResult],
                              original_tasks: List[WorkerTask],
                              context: Optional[PipelineContext] = None) -> List[RepairTask]:
        """Generate repair tasks from build failures.

        Maps each build error to the responsible worker type and creates
        a RepairTask that can be fed back into Stage 2.
        """
        repair_tasks = []

        for build_result in build_results:
            if build_result.success:
                continue

            for error in build_result.errors:
                worker_type_str = error.get("worker_type", "cpp")
                try:
                    worker_type = WorkerType(worker_type_str)
                except ValueError:
                    worker_type = WorkerType.CPP

                repair_task = RepairTask(
                    task_id=f"repair-{len(repair_tasks)}",
                    original_task_id=f"{worker_type_str}-0",
                    worker_type=worker_type,
                    error_description=error.get("match", "Unknown error"),
                    file_paths=[error.get("file", "")],
                    error_log=f"Type: {error.get('type', 'unknown')}\n"
                              f"File: {error.get('file', '?')}:{error.get('line', '?')}\n"
                              f"Match: {error.get('match', '?')}",
                )
                repair_tasks.append(repair_task)

        if not repair_tasks and not all(br.success for br in build_results):
            # Generic repair task if errors weren't classified
            repair_tasks.append(RepairTask(
                task_id="repair-generic",
                original_task_id="cpp-0",
                worker_type=WorkerType.CPP,
                error_description="Unclassified build failure — check build logs",
                error_log="\n".join(br.stderr for br in build_results if not br.success),
            ))

        if context:
            context.add_log("build",
                            f"Generated {len(repair_tasks)} repair tasks from build failures",
                            Severity.WARNING if repair_tasks else Severity.INFO)

        return repair_tasks
