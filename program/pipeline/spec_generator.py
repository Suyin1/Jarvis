# Stage 1: 规约生成器
# 将自然语言需求转化为结构化规约文档

import datetime
import json
import logging
import re
from pathlib import Path
from typing import Optional, List, Dict

from .llm_client import BaseLLMClient, LLMFactory
from .models import (
    Specification, InterfaceContract, TestCase, BuildConfigChange,
    Severity, PipelineContext,
)
from .knowledge_base import KnowledgeBase
from .memory import PipelineMemory

logger = logging.getLogger(__name__)


class SpecGenerator:
    """Stage 1 pipeline component.

    Takes a natural language requirement and produces a structured
    Specification document (the "contract") that all downstream stages
    depend on.
    """

    def __init__(self, config: dict, knowledge_base: KnowledgeBase,
                 memory: Optional[PipelineMemory] = None):
        self.config = config
        pipeline_cfg = config.get("pipeline", {})
        self.llm_config = pipeline_cfg.get("llm", {})
        self.llm: BaseLLMClient = LLMFactory.create(self.llm_config)
        self.knowledge_base = knowledge_base
        self.memory = memory

        # Load prompt template
        self.system_prompt = self._load_prompt("prompts/spec_generator.md")

    def _load_prompt(self, path: str) -> str:
        """Load a prompt template from file."""
        p = Path(path)
        if p.exists():
            return p.read_text(encoding="utf-8")
        logger.warning(f"Prompt file not found: {path}, using default")
        return self._default_system_prompt()

    def _default_system_prompt(self) -> str:
        return """You are a Specification Architect. Your role is to transform vague 
or high-level product requirements into precise, structured specifications that 
can be executed by specialized code generation workers.

You MUST produce a specification that includes:
1. Functional description and interaction flow
2. Interface contracts (C++ headers, NAPI signatures, ArkTS imports)
3. File manifest (what to create/modify)
4. Build configuration changes
5. Test cases (at least 3)

Output your specification as a JSON object with the following structure:
{
  "title": "...",
  "functional_description": "...",
  "interaction_flow": ["step1", "step2", ...],
  "state_changes": [{"trigger": "...", "result": "..."}],
  "interfaces": [
    {
      "module_name": "...",
      "cpp_header_declaration": "...",
      "napi_function_signatures": ["...", "..."],
      "arkts_import_declaration": "...",
      "arkts_method_signatures": ["...", "..."]
    }
  ],
  "files_to_create": ["path/file1.cpp", "path/file2.ets"],
  "files_to_modify": ["path/CMakeLists.txt"],
  "build_config_changes": [
    {"file_path": "...", "change_type": "modify", "content": "...", "description": "..."}
  ],
  "test_cases": [
    {
      "id": "001",
      "description": "...",
      "steps": ["step1", "step2"],
      "expected_result": "...",
      "preconditions": []
    }
  ],
  "constraints": ["..."],
  "dependencies": ["..."]
}
"""

    def generate(self, requirement: str, context: Optional[PipelineContext] = None) -> Specification:
        """Generate a specification from a natural language requirement.

        Args:
            requirement: The natural language requirement.
            context: Optional pipeline context for logging.

        Returns:
            A structured Specification object.
        """
        logger.info(f"Generating specification for: {requirement[:100]}...")
        if context:
            context.add_log("spec_generation", f"Generating spec for: {requirement[:100]}")

        # Assemble enhanced prompt
        kb_context = self.knowledge_base.assemble_system_context()
        few_shot = ""
        if self.memory:
            few_shot = self.memory.assemble_few_shot_examples(requirement)

        user_prompt = self._build_user_prompt(requirement, kb_context, few_shot)
        messages = self.llm.build_messages(self.system_prompt, user_prompt)

        # Call LLM
        response = self.llm.chat_with_retry(messages)
        if not response.success:
            error_msg = f"Spec generation failed: {response.error}"
            logger.error(error_msg)
            if context:
                context.add_log("spec_generation", error_msg, Severity.ERROR)
            raise RuntimeError(error_msg)

        # Parse response
        spec = self._parse_response(response.content, requirement)
        spec.created_at = datetime.datetime.now().isoformat()

        if context:
            context.add_log("spec_generation",
                            f"Spec generated: {spec.title} ({len(spec.interfaces)} interfaces, "
                            f"{len(spec.test_cases)} test cases)")

        return spec

    def _build_user_prompt(self, requirement: str, kb_context: str,
                           few_shot: str) -> str:
        sections = [
            f"# Requirement\n\n{requirement}\n",
        ]
        if kb_context:
            sections.append(f"# Knowledge Base Reference\n\n{kb_context}\n")
        if few_shot:
            sections.append(f"{few_shot}\n")

        sections.append(
            "Generate a complete, detailed specification as a JSON object. "
            "Be precise about interface contracts. "
            "Include at least 3 test cases with clear expected results."
        )
        return "\n\n".join(sections)

    def _parse_response(self, content: str, original_requirement: str) -> Specification:
        """Parse the LLM response into a Specification object."""

        # Try to extract JSON from the response
        json_str = self._extract_json(content)
        if json_str:
            try:
                data = json.loads(json_str)
                return Specification.from_dict(data)
            except json.JSONDecodeError as e:
                logger.warning(f"Failed to parse JSON from LLM response: {e}")

        # Fallback: create minimal spec from the raw text
        logger.warning("Falling back to text-only specification")
        return Specification(
            title=original_requirement[:60],
            description=content[:2000],
            functional_description=content[:2000],
            created_at=datetime.datetime.now().isoformat(),
        )

    def _extract_json(self, text: str) -> Optional[str]:
        """Extract a JSON object from text, handling markdown code blocks."""
        # Try ```json ... ``` block first
        match = re.search(r'```(?:json)?\s*\n?(.*?)\n?```', text, re.DOTALL)
        if match:
            return match.group(1).strip()

        # Try finding top-level {...} with balanced braces
        brace_depth = 0
        start = -1
        for i, ch in enumerate(text):
            if ch == '{':
                if brace_depth == 0:
                    start = i
                brace_depth += 1
            elif ch == '}':
                brace_depth -= 1
                if brace_depth == 0 and start >= 0:
                    return text[start:i + 1]
        return None

    def spec_from_file(self, path: str) -> Specification:
        """Load a specification from a file."""
        p = Path(path)
        if not p.exists():
            raise FileNotFoundError(f"Spec file not found: {path}")

        content = p.read_text(encoding="utf-8")
        # If it's JSON, parse directly
        if path.endswith(".json"):
            data = json.loads(content)
            return Specification.from_dict(data)

        # Otherwise, try to parse as markdown (basic)
        # For now, treat as text
        return Specification(
            title=p.stem,
            description=content[:2000],
            functional_description=content[:2000],
            created_at=datetime.datetime.now().isoformat(),
        )
