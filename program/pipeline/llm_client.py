# LLM 抽象客户端 — 可插拔的多后端支持

import abc
import json
import logging
import os
import time
from dataclasses import dataclass, field
from typing import Optional, List, Dict

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# LLM Response
# ---------------------------------------------------------------------------

@dataclass
class LLMResponse:
    content: str
    model: str = ""
    usage: dict = field(default_factory=dict)
    elapsed_seconds: float = 0.0
    success: bool = True
    error: str = ""


# ---------------------------------------------------------------------------
# Abstract Base Class
# ---------------------------------------------------------------------------

class BaseLLMClient(abc.ABC):
    """Abstract base for all LLM backends."""

    def __init__(self, config: dict):
        self.config = config
        self.model = config.get("model", "gpt-4o")
        self.temperature = config.get("temperature", 0.3)
        self.max_tokens = config.get("max_tokens", 4096)
        self.timeout = config.get("timeout_seconds", 120)
        self.retry_count = config.get("retry_count", 3)
        self.retry_delay = config.get("retry_delay_seconds", 5)

    @abc.abstractmethod
    def chat(self, messages: List[dict], **kwargs) -> LLMResponse:
        """Send a chat completion request."""

    def chat_with_retry(self, messages: List[dict], **kwargs) -> LLMResponse:
        """Send with automatic retry on failure."""
        last_error = ""
        for attempt in range(self.retry_count):
            try:
                return self.chat(messages, **kwargs)
            except Exception as e:
                last_error = str(e)
                logger.warning(f"LLM call failed (attempt {attempt+1}/{self.retry_count}): {e}")
                if attempt < self.retry_count - 1:
                    time.sleep(self.retry_delay)
        return LLMResponse(content="", success=False, error=last_error)

    def build_messages(self, system_prompt: str, user_prompt: str) -> List[dict]:
        return [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt},
        ]


# ---------------------------------------------------------------------------
# OpenAI Backend
# ---------------------------------------------------------------------------

class OpenAIClient(BaseLLMClient):
    """OpenAI API backend."""

    def __init__(self, config: dict):
        super().__init__(config)
        self.api_key = os.environ.get("OPENAI_API_KEY", "")
        self.api_base = os.environ.get("OPENAI_API_BASE", "https://api.openai.com/v1")

    def chat(self, messages: List[dict], **kwargs) -> LLMResponse:
        import openai
        client = openai.OpenAI(api_key=self.api_key, base_url=self.api_base)
        start = time.time()
        response = client.chat.completions.create(
            model=kwargs.get("model", self.model),
            messages=messages,
            temperature=kwargs.get("temperature", self.temperature),
            max_tokens=kwargs.get("max_tokens", self.max_tokens),
            timeout=self.timeout,
        )
        elapsed = time.time() - start
        choice = response.choices[0]
        return LLMResponse(
            content=choice.message.content or "",
            model=response.model,
            usage=dict(response.usage) if response.usage else {},
            elapsed_seconds=elapsed,
        )


# ---------------------------------------------------------------------------
# Anthropic Backend
# ---------------------------------------------------------------------------

class AnthropicClient(BaseLLMClient):
    """Anthropic API backend."""

    def __init__(self, config: dict):
        super().__init__(config)
        self.api_key = os.environ.get("ANTHROPIC_API_KEY", "")

    def chat(self, messages: List[dict], **kwargs) -> LLMResponse:
        import anthropic
        client = anthropic.Anthropic(api_key=self.api_key)
        start = time.time()

        # Extract system message
        system = ""
        chat_messages = []
        for m in messages:
            if m["role"] == "system":
                system = m["content"]
            else:
                chat_messages.append({"role": m["role"], "content": m["content"]})

        response = client.messages.create(
            model=kwargs.get("model", self.model),
            system=system,
            messages=chat_messages,
            max_tokens=kwargs.get("max_tokens", self.max_tokens),
            temperature=kwargs.get("temperature", self.temperature),
        )
        elapsed = time.time() - start
        return LLMResponse(
            content=response.content[0].text if response.content else "",
            model=response.model,
            usage={"input_tokens": response.usage.input_tokens, "output_tokens": response.usage.output_tokens},
            elapsed_seconds=elapsed,
        )


# ---------------------------------------------------------------------------
# Azure OpenAI Backend
# ---------------------------------------------------------------------------

class AzureClient(BaseLLMClient):
    """Azure OpenAI backend."""

    def __init__(self, config: dict):
        super().__init__(config)
        self.api_key = os.environ.get("AZURE_API_KEY", "")
        self.api_base = os.environ.get("AZURE_API_BASE", "")
        self.api_version = os.environ.get("AZURE_API_VERSION", "2024-02-01")

    def chat(self, messages: List[dict], **kwargs) -> LLMResponse:
        import openai
        client = openai.AzureOpenAI(
            api_key=self.api_key,
            api_version=self.api_version,
            azure_endpoint=self.api_base,
        )
        start = time.time()
        response = client.chat.completions.create(
            model=kwargs.get("model", self.model),
            messages=messages,
            temperature=kwargs.get("temperature", self.temperature),
            max_tokens=kwargs.get("max_tokens", self.max_tokens),
        )
        elapsed = time.time() - start
        choice = response.choices[0]
        return LLMResponse(
            content=choice.message.content or "",
            model=response.model,
            usage=dict(response.usage) if response.usage else {},
            elapsed_seconds=elapsed,
        )


# ---------------------------------------------------------------------------
# Local / Mock Backend (for testing without API keys)
# ---------------------------------------------------------------------------

class MockClient(BaseLLMClient):
    """Mock backend for testing without real LLM calls."""

    def __init__(self, config: dict):
        super().__init__(config)
        self.response_map = {}

    def register_response(self, prompt_pattern: str, response: str) -> None:
        self.response_map[prompt_pattern] = response

    def chat(self, messages: List[dict], **kwargs) -> LLMResponse:
        user_content = ""
        for m in messages:
            if m["role"] == "user":
                user_content += m["content"] + "\n"

        # Find matching response
        content = self._default_json_response()
        for pattern, resp in self.response_map.items():
            if pattern in user_content:
                content = resp
                break

        return LLMResponse(content=content, model="mock", elapsed_seconds=0.01)

    def _default_json_response(self) -> str:
        return """{
  "title": "Mock Feature: Echo Module",
  "version": "1.0",
  "description": "An echo module that sends text from ArkTS to QT via NAPI",
  "functional_description": "The user enters text in an ArkTS TextInput. When a button is pressed, the text is sent via NAPI bridge to a QT C++ module that processes it (echoes back with a prefix) and returns the result to ArkTS for display.",
  "interaction_flow": [
    "User opens the app and sees a TextInput and a Button",
    "User types text and clicks 'Process' button",
    "ArkTS calls nativeModule.echoText(input)",
    "NAPI bridge validates the input string",
    "QT C++ echoModule receives the string and returns 'Echo: ' + input",
    "NAPI converts the result to Napi::String",
    "ArkTS receives the result and displays it in a Text component"
  ],
  "state_changes": [
    {"trigger": "User clicks Process button", "result": "Result text updates with echoed string"}
  ],
  "interfaces": [
    {
      "module_name": "EchoModule",
      "cpp_header_declaration": "class EchoModule : public QObject { Q_OBJECT public: explicit EchoModule(QObject *parent = nullptr); QString echoText(const QString &input); signals: void echoCompleted(const QString &result); };",
      "napi_function_signatures": ["napi_value EchoText(napi_env env, napi_callback_info info)"],
      "arkts_import_declaration": "import echoModule from 'libecho.so'",
      "arkts_method_signatures": ["echoText(input: string): string"]
    }
  ],
  "files_to_create": [
    "src/cpp/echo_module.h",
    "src/cpp/echo_module.cpp",
    "src/napi/echo_bridge.cpp",
    "src/ets/pages/EchoFeature.ets"
  ],
  "files_to_modify": [
    "CMakeLists.txt",
    "oh-package.json5"
  ],
  "build_config_changes": [
    {"file_path": "CMakeLists.txt", "change_type": "modify", "content": "target_sources(myapp PRIVATE src/cpp/echo_module.cpp src/napi/echo_bridge.cpp)", "description": "Add echo module sources"}
  ],
  "test_cases": [
    {"id": "001", "description": "Echo text from ArkTS to QT and back", "steps": ["Launch app", "Enter 'Hello' in TextInput", "Click Process button"], "expected_result": "Text component displays 'Echo: Hello'", "preconditions": ["App is running on emulator or device"]},
    {"id": "002", "description": "Empty input handling", "steps": ["Launch app", "Leave TextInput empty", "Click Process button"], "expected_result": "Displays 'Echo: ' or appropriate empty message", "preconditions": []},
    {"id": "003", "description": "Special characters", "steps": ["Launch app", "Enter '!@#$%' in TextInput", "Click Process button"], "expected_result": "Displays 'Echo: !@#$%' without errors", "preconditions": []}
  ],
  "constraints": ["Must support HarmonyOS 4.0+", "QT 6.5+", "NAPI thread-safe calls"],
  "dependencies": ["@ohos/napi"]
}"""


# ---------------------------------------------------------------------------
# Factory
# ---------------------------------------------------------------------------

class LLMFactory:
    """Factory to create LLM clients by backend type."""

    _backends = {
        "openai": OpenAIClient,
        "anthropic": AnthropicClient,
        "azure": AzureClient,
        "mock": MockClient,
        "local": OpenAIClient,  # Local LLMs often use OpenAI-compatible API
    }

    @classmethod
    def register_backend(cls, name: str, backend_cls: type) -> None:
        cls._backends[name] = backend_cls

    @classmethod
    def create(cls, config: dict) -> BaseLLMClient:
        backend = config.get("backend", "openai")
        if backend not in cls._backends:
            raise ValueError(f"Unknown LLM backend: {backend}. Available: {list(cls._backends.keys())}")
        return cls._backends[backend](config)
