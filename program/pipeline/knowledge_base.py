# 知识库管理系统（Stage 0）

import logging
from pathlib import Path
from typing import Optional, List, Dict

logger = logging.getLogger(__name__)


class KnowledgeBase:
    """Manages the knowledge base assets for the pipeline.

    The knowledge base stores reusable documentation, templates, and learned
    patterns that guide all LLM generations. This is Stage 0 of the pipeline.
    """

    def __init__(self, kb_path: str = "knowledge"):
        self.kb_path = Path(kb_path)
        self.kb_path.mkdir(parents=True, exist_ok=True)
        (self.kb_path / "templates").mkdir(parents=True, exist_ok=True)

    # ------------------------------------------------------------------
    # Document management
    # ------------------------------------------------------------------

    def list_documents(self) -> List[Path]:
        """List all markdown documents in the knowledge base."""
        return sorted(self.kb_path.rglob("*.md"))

    def read_document(self, name: str) -> str:
        """Read a knowledge base document by name (relative to kb_path)."""
        path = self.kb_path / name
        if not path.exists():
            logger.warning(f"Knowledge document not found: {path}")
            return ""
        return path.read_text(encoding="utf-8")

    def write_document(self, name: str, content: str) -> Path:
        """Write a document to the knowledge base."""
        path = self.kb_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        logger.info(f"Knowledge document written: {path}")
        return path

    # ------------------------------------------------------------------
    # Template management
    # ------------------------------------------------------------------

    def get_template(self, name: str) -> str:
        """Read a template file."""
        return self.read_document(f"templates/{name}")

    def save_template(self, name: str, content: str) -> Path:
        """Save a template file."""
        return self.write_document(f"templates/{name}", content)

    # ------------------------------------------------------------------
    # Context assembly (for LLM prompts)
    # ------------------------------------------------------------------

    def assemble_system_context(self, max_tokens_hint: int = 4000) -> str:
        """Assemble a system-level context from the knowledge base.

        This builds a consolidated context string that can be injected into
        LLM system prompts, ensuring all generations are guided by accumulated
        knowledge.
        """
        sections = [
            "# Knowledge Base Context",
            "",
            "The following is reference knowledge for generating ArkTS + QT bridge code.",
            "All generated code MUST adhere to the patterns and practices documented here.",
            "",
        ]

        for doc_path in self.list_documents():
            if doc_path.name == "templates":
                continue
            content = doc_path.read_text(encoding="utf-8")
            sections.append(f"---\n## Source: {doc_path.relative_to(self.kb_path)}\n")
            sections.append(content[:max_tokens_hint])

        return "\n".join(sections)

    # ------------------------------------------------------------------
    # Search
    # ------------------------------------------------------------------

    def search(self, query: str, max_results: int = 5) -> List[dict]:
        """Simple keyword-based search across knowledge base documents."""
        query_lower = query.lower()
        results = []
        for doc_path in self.list_documents():
            content = doc_path.read_text(encoding="utf-8")
            if query_lower in content.lower():
                # Find relevant snippets
                lines = content.split("\n")
                snippets = []
                for i, line in enumerate(lines):
                    if query_lower in line.lower():
                        start = max(0, i - 2)
                        end = min(len(lines), i + 3)
                        snippets.append("\n".join(lines[start:end]))
                results.append({
                    "path": str(doc_path),
                    "snippets": snippets[:3],
                    "score": content.lower().count(query_lower),
                })
        results.sort(key=lambda x: x["score"], reverse=True)
        return results[:max_results]
