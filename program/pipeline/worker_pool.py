# Stage 2: 并行代码生成 Worker 池
# 管理并发 Worker 进程，生成各模块代码

import concurrent.futures
import logging
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional, List, Dict

from .llm_client import BaseLLMClient, LLMFactory
from .models import (
    WorkerType, WorkerTask, WorkerStatus, Specification,
    Severity, PipelineContext,
)
from .knowledge_base import KnowledgeBase

logger = logging.getLogger(__name__)


class CodeWorker:
    """A single code generation worker that handles one module type.

    Each worker is a self-contained process that reads the specification
    and generates code for its assigned module.
    """

    def __init__(self, worker_type: WorkerType, config: dict,
                 knowledge_base: KnowledgeBase):
        self.worker_type = worker_type
        self.config = config
        pipeline_cfg = config.get("pipeline", {})
        self.llm_config = pipeline_cfg.get("llm", {})
        self.llm: BaseLLMClient = LLMFactory.create(self.llm_config)
        self.knowledge_base = knowledge_base
        workers_cfg = pipeline_cfg.get("workers", {})
        self.output_dir = workers_cfg.get(worker_type.value, {}).get("output_dir", f"output/{worker_type.value}")

        # Load worker-specific prompt
        prompt_path = workers_cfg.get(worker_type.value, {}).get("prompt_template", "")
        self.system_prompt = self._load_prompt(prompt_path, worker_type)

    def _load_prompt(self, path: str, worker_type: WorkerType) -> str:
        p = Path(path)
        if p.exists():
            return p.read_text(encoding="utf-8")
        # Return default prompt if file not found
        return self._default_prompt(worker_type)

    def _default_prompt(self, worker_type: WorkerType) -> str:
        prompts = {
            WorkerType.CPP: """You are a QT/C++ expert. Generate production-quality C++ code
for the QT side of an ArkTS-QT bridge application.
Follow the interface contract strictly. Output complete .cpp and .h files.""",
            WorkerType.NAPI: """You are a NAPI bridge expert. Generate NAPI C++ wrapper code
that bridges QT C++ code to ArkTS. Ensure proper type conversions,
thread safety, and error handling.""",
            WorkerType.ARKTS: """You are an ArkTS UI expert. Generate ArkTS/TypeScript code
for HarmonyOS applications. Follow the declared interface contracts
for importing and calling native modules.""",
            WorkerType.CONFIG: """You are a build configuration expert. Generate and update
CMakeLists.txt, oh-package.json5, and other build configuration files
for QT + HarmonyOS hybrid projects.""",
        }
        return prompts.get(worker_type, "Generate high-quality code per the specification.")

    def execute(self, task: WorkerTask, context: Optional[PipelineContext] = None) -> WorkerTask:
        """Execute a code generation task.

        Args:
            task: The worker task with specification.
            context: Optional pipeline context for logging.

        Returns:
            The task with updated status and results.
        """
        logger.info(f"Worker [{self.worker_type.value}] executing task: {task.task_id}")
        task.status = WorkerStatus.RUNNING

        # Build prompt with specification context
        spec = task.specification
        spec_markdown = spec.to_markdown()
        kb_context = self.knowledge_base.assemble_system_context()

        # Add worker-specific rules
        worker_rules = self._get_worker_rules()
        output_constraints = self._get_output_constraints()

        user_prompt = (
            f"# Specification\n\n{spec_markdown}\n\n"
            f"# Knowledge Base\n\n{kb_context}\n\n"
            f"# Worker Rules\n\n{worker_rules}\n\n"
            f"# Output Constraints\n\n{output_constraints}\n\n"
            f"Generate the complete code for module type: {self.worker_type.value}.\n"
            f"Output each file with a '--- filename' separator."
        )

        messages = self.llm.build_messages(self.system_prompt, user_prompt)
        response = self.llm.chat_with_retry(messages)

        if not response.success:
            task.status = WorkerStatus.FAILED
            task.error_message = response.error
            if context:
                context.add_log("code_generation",
                                f"Worker [{self.worker_type.value}] failed: {response.error}",
                                Severity.ERROR)
            return task

        # Extract and save generated files
        output_path = Path(task.output_dir or self.output_dir)
        files_written = self._save_generated_files(response.content, output_path)

        task.status = WorkerStatus.SUCCESS
        task.result_files = files_written
        task.attempt_count += 1

        if context:
            context.add_log("code_generation",
                            f"Worker [{self.worker_type.value}] completed: {len(files_written)} files written")

        return task

    def _get_worker_rules(self) -> str:
        """Get worker-specific generation rules."""
        rules = {
            WorkerType.CPP: (
                "- Generate complete .cpp and .h files\n"
                "- All public methods must have documentation\n"
                "- Follow QT coding conventions\n"
                "- Include necessary QT headers\n"
                "- Never modify ArkTS or NAPI files"
            ),
            WorkerType.NAPI: (
                "- Generate complete NAPI bridge files\n"
                "- Handle all type conversions explicitly\n"
                "- Include error checking for all NAPI calls\n"
                "- Ensure thread-safe callbacks\n"
                "- Never modify QT logic or ArkTS UI files"
            ),
            WorkerType.ARKTS: (
                "- Generate complete .ets files\n"
                "- Use HarmonyOS recommended component patterns\n"
                "- Follow ArkTS coding conventions\n"
                "- Import native modules correctly\n"
                "- Never modify C++ files"
            ),
            WorkerType.CONFIG: (
                "- Generate valid CMakeLists.txt\n"
                "- Generate valid oh-package.json5\n"
                "- All paths relative to project root\n"
                "- Include only necessary dependencies\n"
                "- Never modify source code files"
            ),
        }
        return rules.get(self.worker_type, "- Follow the specification exactly")

    def _get_output_constraints(self) -> str:
        constraints = {
            WorkerType.CPP: "Output files go to: output/cpp/",
            WorkerType.NAPI: "Output files go to: output/napi/",
            WorkerType.ARKTS: "Output files go to: output/ets/",
            WorkerType.CONFIG: "Output files go to: output/config/",
        }
        return constraints.get(self.worker_type, "Output files to the designated directory.")

    def _save_generated_files(self, content: str, output_dir: Path) -> List[str]:
        """Parse generated code blocks and save to files.

        Expects format: '--- filename\ncode content' separators.
        """
        import re
        output_dir.mkdir(parents=True, exist_ok=True)
        files_written = []

        # Parse file blocks: --- filename.ext\ncode
        blocks = re.split(r'---\s*([^\n]+)', content)
        if len(blocks) < 2:
            # Single file output — guess a default filename
            default_name = f"{self.worker_type.value}_generated"
            ext_map = {
                WorkerType.CPP: ".cpp",
                WorkerType.NAPI: ".cpp",
                WorkerType.ARKTS: ".ets",
                WorkerType.CONFIG: ".txt",
            }
            default_name += ext_map.get(self.worker_type, ".txt")
            file_path = output_dir / default_name
            file_path.write_text(content, encoding="utf-8")
            files_written.append(str(file_path))
            return files_written

        # Process each block
        for i in range(1, len(blocks), 2):
            filename = blocks[i].strip()
            code = blocks[i + 1].strip() if i + 1 < len(blocks) else ""

            # Remove markdown code fences if present
            code = re.sub(r'^```\w*\n', '', code, flags=re.MULTILINE)
            code = re.sub(r'\n```$', '', code, flags=re.MULTILINE)

            file_path = output_dir / filename
            file_path.parent.mkdir(parents=True, exist_ok=True)
            file_path.write_text(code, encoding="utf-8")
            files_written.append(str(file_path))
            logger.info(f"  Written: {file_path}")

        return files_written


class WorkerPool:
    """Manages parallel code generation workers (Stage 2 orchestrator)."""

    def __init__(self, config: dict, knowledge_base: KnowledgeBase):
        self.config = config
        self.knowledge_base = knowledge_base
        self.max_workers = config.get("pipeline", {}).get("max_concurrent_workers", 4)

    def create_tasks(self, spec: Specification, context: PipelineContext) -> List[WorkerTask]:
        """Create worker tasks from a specification.

        Determines which workers are needed based on the spec content.
        """
        workers_config = self.config.get("pipeline", {}).get("workers", {})
        tasks = []

        worker_mapping = {
            WorkerType.CPP: ("cpp", workers_config.get("cpp", {})),
            WorkerType.NAPI: ("napi", workers_config.get("napi", {})),
            WorkerType.ARKTS: ("arkts", workers_config.get("arkts", {})),
            WorkerType.CONFIG: ("config", workers_config.get("config", {})),
        }

        task_index = 0
        for wtype, (key, wconfig) in worker_mapping.items():
            if not wconfig.get("enabled", True):
                continue

            # Determine if this worker is needed based on spec
            if not self._is_worker_needed(wtype, spec):
                logger.info(f"Worker [{wtype.value}] not needed for this spec, skipping")
                continue

            count = wconfig.get("count", 1)
            for i in range(count):
                task_id = f"{wtype.value}-{task_index}"
                output_dir = wconfig.get("output_dir", f"output/{wtype.value}")
                tasks.append(WorkerTask(
                    task_id=task_id,
                    worker_type=wtype,
                    specification=spec,
                    output_dir=output_dir,
                    status=WorkerStatus.PENDING,
                ))
                task_index += 1

        context.worker_outputs = {
            "cpp": [], "napi": [], "arkts": [], "config": []
        }
        return tasks

    def _is_worker_needed(self, worker_type: WorkerType, spec: Specification) -> bool:
        """Determine if a worker type is needed based on the specification."""
        if worker_type == WorkerType.CPP:
            return bool(spec.files_to_create or spec.files_to_modify)
        if worker_type == WorkerType.NAPI:
            return any(i.napi_function_signatures for i in spec.interfaces)
        if worker_type == WorkerType.ARKTS:
            return any(i.arkts_method_signatures for i in spec.interfaces)
        if worker_type == WorkerType.CONFIG:
            return len(spec.build_config_changes) > 0 or any(
                "CMakeLists" in f or "package" in f or "config" in f
                for f in spec.files_to_create + spec.files_to_modify
            )
        return True

    def execute_all(self, tasks: List[WorkerTask],
                    context: Optional[PipelineContext] = None) -> List[WorkerTask]:
        """Execute all worker tasks in parallel using a thread pool."""
        if not tasks:
            logger.info("No worker tasks to execute")
            return tasks

        logger.info(f"Executing {len(tasks)} worker tasks (max_parallel={self.max_workers})")
        if context:
            context.add_log("code_generation",
                            f"Starting {len(tasks)} parallel workers")

        completed_tasks = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            future_to_task = {}
            for task in tasks:
                worker = CodeWorker(task.worker_type, self.config, self.knowledge_base)
                future = executor.submit(worker.execute, task, context)
                future_to_task[future] = task

            for future in concurrent.futures.as_completed(future_to_task):
                original_task = future_to_task[future]
                try:
                    completed = future.result()
                    completed_tasks.append(completed)
                    # Update context
                    if context:
                        wtype = completed.worker_type.value
                        context.worker_outputs[wtype].extend(completed.result_files)
                except Exception as e:
                    logger.error(f"Worker [{original_task.worker_type.value}] raised exception: {e}")
                    original_task.status = WorkerStatus.FAILED
                    original_task.error_message = str(e)
                    completed_tasks.append(original_task)

        # Report results
        success_count = sum(1 for t in completed_tasks if t.status == WorkerStatus.SUCCESS)
        fail_count = sum(1 for t in completed_tasks if t.status == WorkerStatus.FAILED)
        logger.info(f"Workers completed: {success_count} success, {fail_count} failed")

        if context:
            context.add_log("code_generation",
                            f"Workers done: {success_count} success, {fail_count} failed")

        return completed_tasks
