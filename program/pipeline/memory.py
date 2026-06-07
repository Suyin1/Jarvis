# 记忆与进化系统 — 跨流水线运行累积知识

import datetime
import json
import logging
from pathlib import Path
from typing import Optional, List, Dict

from .models import MemoryEntry

logger = logging.getLogger(__name__)


class PipelineMemory:
    """Persistent memory that evolves the pipeline's capabilities over time.

    After each successful pipeline run, key lessons, failure patterns, and
    success patterns are stored. These entries serve as few-shot examples for
    future generations, reducing error rates over time.
    """

    def __init__(self, memory_path: str = "memory", max_entries: int = 100):
        self.memory_path = Path(memory_path)
        self.memory_path.mkdir(parents=True, exist_ok=True)
        self.max_entries = max_entries
        self.index_path = self.memory_path / "index.json"
        self._entries: List[MemoryEntry] = []
        self._load_index()

    # ------------------------------------------------------------------
    # Index Management
    # ------------------------------------------------------------------

    def _load_index(self) -> None:
        """Load the memory index from disk."""
        if self.index_path.exists():
            try:
                data = json.loads(self.index_path.read_text(encoding="utf-8"))
                self._entries = [MemoryEntry(**e) for e in data]
            except Exception as e:
                logger.warning(f"Failed to load memory index: {e}")
                self._entries = []

    def _save_index(self) -> None:
        """Save the memory index to disk."""
        # Prune if needed
        if len(self._entries) > self.max_entries:
            self._entries = self._entries[-self.max_entries:]
        data = [e.__dict__ for e in self._entries]
        self.index_path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")

    # ------------------------------------------------------------------
    # Entry Management
    # ------------------------------------------------------------------

    def add_entry(self, entry: MemoryEntry) -> None:
        """Add a new memory entry."""
        if not entry.entry_id:
            entry.entry_id = f"mem-{datetime.datetime.now().strftime('%Y%m%d%H%M%S')}-{len(self._entries)}"
        if not entry.created_at:
            entry.created_at = datetime.datetime.now().isoformat()
        self._entries.append(entry)
        self._save_index()
        logger.info(f"Memory entry added: {entry.entry_id} — {entry.title}")

        # Save full entry as individual file
        entry_path = self.memory_path / f"{entry.entry_id}.json"
        entry_path.write_text(json.dumps(entry.__dict__, ensure_ascii=False, indent=2), encoding="utf-8")

    def get_entry(self, entry_id: str) -> Optional[MemoryEntry]:
        """Retrieve a specific memory entry."""
        for e in self._entries:
            if e.entry_id == entry_id:
                return e
        return None

    def list_entries(self, tags: Optional[List[str]] = None) -> List[MemoryEntry]:
        """List all entries, optionally filtered by tags."""
        if not tags:
            return self._entries
        return [e for e in self._entries if any(t in e.tags for t in tags)]

    # ------------------------------------------------------------------
    # Similarity Search
    # ------------------------------------------------------------------

    def find_similar(self, requirement: str, threshold: float = 0.3, max_results: int = 3) -> List[MemoryEntry]:
        """Find memory entries similar to a given requirement using keyword overlap.

        This is a simple bag-of-words approach. In production, replace with
        embedding-based similarity search (e.g., sentence-transformers).
        """
        req_words = set(requirement.lower().split())
        scored: List[tuple[float, MemoryEntry]] = []

        for entry in self._entries:
            entry_words = set(entry.requirement.lower().split()) | set(entry.spec_summary.lower().split())
            if not entry_words:
                continue
            overlap = len(req_words & entry_words) / max(len(req_words | entry_words), 1)
            if overlap >= threshold:
                scored.append((overlap, entry))

        scored.sort(key=lambda x: x[0], reverse=True)
        return [e for _, e in scored[:max_results]]

    # ------------------------------------------------------------------
    # Few-shot Assembly
    # ------------------------------------------------------------------

    def assemble_few_shot_examples(self, requirement: str, max_examples: int = 2) -> str:
        """Assemble few-shot example strings from similar past entries.

        This can be injected into LLM prompts to improve generation quality.
        """
        similar = self.find_similar(requirement, max_results=max_examples)
        if not similar:
            return ""

        sections = ["# Similar Past Examples (Few-Shot)", ""]
        for entry in similar:
            sections.extend([
                f"## Example: {entry.title}",
                f"**Requirement:** {entry.requirement}",
                f"**Summary:** {entry.spec_summary}",
                "**Key Lessons:**",
            ])
            sections.extend(f"- {l}" for l in entry.key_lessons)
            sections.append("")

        return "\n".join(sections)

    # ------------------------------------------------------------------
    # Evolution
    # ------------------------------------------------------------------

    def extract_lessons(self, requirement: str, spec_summary: str,
                        failures: List[dict], successes: List[dict]) -> MemoryEntry:
        """Create a memory entry from a completed pipeline run."""
        import uuid
        return MemoryEntry(
            entry_id=f"mem-{uuid.uuid4().hex[:8]}",
            title=requirement[:80],
            requirement=requirement,
            spec_summary=spec_summary,
            key_lessons=[f.get("lesson", "") for f in failures if f.get("lesson")],
            failure_patterns=failures,
            success_patterns=successes,
            created_at=datetime.datetime.now().isoformat(),
            tags=self._extract_tags(requirement),
        )

    def _extract_tags(self, text: str) -> List[str]:
        """Extract simple tags from text."""
        tags = []
        keywords = {
            "qt": ["qt", "qwidget", "qml", "signal", "slot"],
            "arkts": ["arkts", "ets", "typescript", "harmony"],
            "napi": ["napi", "bridge", "native"],
            "cmake": ["cmake", "build", "compile"],
            "ui": ["ui", "layout", "component", "page"],
        }
        text_lower = text.lower()
        for tag, words in keywords.items():
            if any(w in text_lower for w in words):
                tags.append(tag)
        return tags
