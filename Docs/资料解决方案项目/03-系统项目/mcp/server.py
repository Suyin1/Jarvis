"""MCP Server for Doc Solution System

Implements the Model Context Protocol server over stdio transport.
Wraps CLI commands as MCP tools for AI Agent integration.
Zero network dependencies - communicates via stdin/stdout.

Tools:
  - quality_check    : Run quality checks on documents/code
  - generate_content : Generate document content from templates
  - build_knowledge  : Build knowledge base from customer input
"""

import traceback
from pathlib import Path

from mcp.protocol import (
    StdioTransport,
    create_response, create_error,
    PARSE_ERROR, METHOD_NOT_FOUND, INVALID_PARAMS, INTERNAL_ERROR,
)

from tools.check import run_check
from tools.generate import run_generate
from tools.build_kb import run_build_kb

_PROJECT_ROOT = Path(__file__).resolve().parent.parent


class DocSolutionServer(object):
    """MCP Server exposing doc-solution tools via stdio transport."""

    def __init__(self):
        self.transport = StdioTransport()
        self.initialized = False
        self._tools = self._define_tools()

    def _define_tools(self):
        return [
            {
                "name": "quality_check",
                "description": "Run quality checks on documents/code. Supports structure, format, and style checks. Returns a JSON report with summary and details.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "target": {
                            "type": "string",
                            "description": "Target file or directory path to check",
                        },
                        "check_type": {
                            "type": "string",
                            "enum": ["all", "format", "style", "structure"],
                            "description": "Type of check to perform (default: all)",
                        },
                        "vale_bin": {
                            "type": "string",
                            "description": "Vale executable path (default: vale)",
                        },
                        "config_path": {
                            "type": "string",
                            "description": "Vale config file path (.vale.ini)",
                        },
                    },
                    "required": ["target"],
                },
            },
            {
                "name": "generate_content",
                "description": "Generate document content from Jinja2 templates. Supports named templates (api-ref, dev-guide) or custom template files.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "template": {
                            "type": "string",
                            "description": "Template name (e.g. api-ref, dev-guide) or template file path",
                        },
                        "params": {
                            "type": "object",
                            "description": "Template parameters as key-value pairs",
                        },
                        "template_dir": {
                            "type": "string",
                            "description": "Template directory path (default: knowledge/templates)",
                        },
                        "output": {
                            "type": "string",
                            "description": "Output file path (omit to return content in response)",
                        },
                        "auto_check": {
                            "type": "boolean",
                            "description": "Auto run quality check on generated content (default: true)",
                        },
                    },
                    "required": ["template"],
                },
            },
            {
                "name": "build_knowledge",
                "description": "Build a customer-specific knowledge base from source materials. Analyzes document style, generates Vale rules, and creates template configurations.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "input_dir": {
                            "type": "string",
                            "description": "Directory containing customer source materials (docs, templates, etc.)",
                        },
                        "name": {
                            "type": "string",
                            "description": "Customer name identifier (e.g. Huawei-HarmonyOS)",
                        },
                        "output": {
                            "type": "string",
                            "description": "Output directory for knowledge base (default: knowledge)",
                        },
                        "force": {
                            "type": "boolean",
                            "description": "Overwrite existing knowledge base if exists (default: false)",
                        },
                    },
                    "required": ["input_dir", "name"],
                },
            },
        ]

    def run(self):
        """Main loop: read JSON-RPC requests from stdin, write responses to stdout."""
        while True:
            try:
                message = self.transport.read_message()
            except Exception as e:
                self.transport.send_message(
                    create_error(PARSE_ERROR, "Message parse error: %s" % e))
                continue

            if message is None:
                break

            self._handle_message(message)

    def _handle_message(self, message):
        method = message.get("method")
        msg_id = message.get("id")
        params = message.get("params", {})

        if not method:
            self._send_error(INVALID_REQUEST, "Method is required", msg_id)
            return

        if method == "initialize":
            self._handle_initialize(params, msg_id)
        elif method == "notifications/initialized":
            self.initialized = True
        elif method == "tools/list":
            self._handle_list_tools(msg_id)
        elif method == "tools/call":
            self._handle_call_tool(params, msg_id)
        else:
            self._send_error(METHOD_NOT_FOUND,
                             "Unknown method: %s" % method, msg_id)

    def _handle_initialize(self, params, msg_id):
        self.initialized = True
        result = {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {},
            },
            "serverInfo": {
                "name": "doc-solution",
                "version": "0.1.0",
            },
        }
        self.transport.send_message(create_response(result, msg_id))

    def _handle_list_tools(self, msg_id):
        self.transport.send_message(
            create_response({"tools": self._tools}, msg_id))

    def _handle_call_tool(self, params, msg_id):
        name = params.get("name")
        arguments = params.get("arguments", {})

        if not name:
            self._send_error(INVALID_PARAMS, "Tool name is required", msg_id)
            return

        try:
            text = self._execute_tool(name, arguments)
            self.transport.send_message(create_response({
                "content": [{"type": "text", "text": text}],
            }, msg_id))
        except ValueError as e:
            self._send_error(INVALID_PARAMS, str(e), msg_id)
        except Exception as e:
            self.transport.log("Tool error [%s]: %s" % (name, e))
            self.transport.log(traceback.format_exc())
            self._send_error(INTERNAL_ERROR,
                             "Tool execution failed: %s" % e, msg_id)

    def _execute_tool(self, name, args):
        if name == "quality_check":
            target = args.get("target")
            if not target:
                raise ValueError("'target' is required for quality_check")
            report = run_check(
                target=target,
                check_type=args.get("check_type", "all"),
                output_format="json",
                vale_bin=args.get("vale_bin", "vale"),
                config_path=args.get("config_path"),
            )
            return report.to_json()

        elif name == "generate_content":
            content, check_report = run_generate(
                template=args.get("template", ""),
                params_dict=args.get("params", {}),
                template_dir=args.get("template_dir"),
                output=args.get("output"),
                auto_check=args.get("auto_check", True),
            )
            result = content
            if check_report:
                items = check_report.to_dict().get("details", [])
                if items:
                    summary_lines = []
                    summary_lines.append("")
                    summary_lines.append("--- Quality Check ---")
                    for item in items:
                        summary_lines.append("  [%s] %s" % (
                            item.get("severity", "?"), item.get("message", "")))
                    result += "\n".join(summary_lines)
            return result

        elif name == "build_knowledge":
            info = run_build_kb(
                input_dir=args.get("input_dir", ""),
                name=args.get("name", ""),
                output=args.get("output"),
                force=args.get("force", False),
            )
            lines = []
            lines.append("Knowledge base built successfully:")
            lines.append("  Name: %s" % info["name"])
            lines.append("  Output: %s" % info["output_dir"])
            lines.append("  Config: %s" % info["config_file"])
            lines.append("  Files analyzed: %d" % info["file_count"])
            lines.append("  Templates registered: %d" % info["template_count"])
            return "\n".join(lines)

        else:
            raise ValueError("Unknown tool: %s" % name)

    def _send_error(self, code, message, msg_id):
        self.transport.send_message(create_error(code, message, msg_id))


def main():
    """Entry point for running the MCP server."""
    server = DocSolutionServer()
    server.run()


if __name__ == "__main__":
    main()
