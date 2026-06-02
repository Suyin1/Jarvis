"""MCP Protocol - JSON-RPC 2.0 over stdio

Implements the Model Context Protocol transport layer.
Uses Content-Length header framing for message boundaries.
"""

import json
import sys

# --- Error codes ---
PARSE_ERROR = -32700
INVALID_REQUEST = -32600
METHOD_NOT_FOUND = -32601
INVALID_PARAMS = -32602
INTERNAL_ERROR = -32603


class MCPError(Exception):
    def __init__(self, code, message, data=None):
        self.code = code
        self.message = message
        self.data = data

    def to_dict(self):
        result = {"code": self.code, "message": self.message}
        if self.data is not None:
            result["data"] = self.data
        return result


def create_response(result, request_id):
    return {"jsonrpc": "2.0", "result": result, "id": request_id}


def create_error(code, message, request_id=None, data=None):
    msg = {"jsonrpc": "2.0", "error": {"code": code, "message": message}}
    if request_id is not None:
        msg["id"] = request_id
    if data is not None:
        msg["error"]["data"] = data
    return msg


def create_notification(method, params=None):
    msg = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        msg["params"] = params
    return msg


class StdioTransport(object):
    """MCP stdio transport with Content-Length framing."""

    def __init__(self):
        self.stdin = sys.stdin
        self.stdout = sys.stdout
        self.stderr = sys.stderr
        self._buffer = ""

    def read_message(self):
        content_length = 0

        while True:
            line = self.stdin.readline()
            if not line:
                return None
            line = line.strip("\r\n")
            if line == "":
                break
            key, _, value = line.partition(":")
            if key.strip().lower() == "content-length":
                content_length = int(value.strip())

        if content_length == 0:
            return None

        body = self.stdin.read(content_length)
        try:
            return json.loads(body)
        except ValueError as e:
            raise MCPError(PARSE_ERROR, "Parse error: %s" % e)

    def send_message(self, message):
        body = json.dumps(message, ensure_ascii=False)
        raw = body.encode("utf-8")
        header = "Content-Length: %d\r\n\r\n" % len(raw)
        self.stdout.write(header)
        self.stdout.write(body)
        self.stdout.flush()

    def log(self, message):
        self.stderr.write("[MCP] %s\n" % message)
        self.stderr.flush()
