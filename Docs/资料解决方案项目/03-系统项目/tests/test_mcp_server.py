"""MCP Server tests

Tests the JSON-RPC 2.0 protocol layer and MCP server functionality.
"""

import json
try:
    from io import StringIO
except ImportError:
    from StringIO import StringIO

from mcp.protocol import (
    StdioTransport, MCPError,
    create_response, create_error, create_notification,
    PARSE_ERROR, INVALID_REQUEST, METHOD_NOT_FOUND, INVALID_PARAMS, INTERNAL_ERROR,
)


class TestProtocol(object):
    """Test JSON-RPC 2.0 protocol message construction."""

    def test_create_response(self):
        msg = create_response({"ok": True}, "req-1")
        assert msg["jsonrpc"] == "2.0"
        assert msg["result"]["ok"] is True
        assert msg["id"] == "req-1"

    def test_create_error(self):
        msg = create_error(-32601, "Method not found", "req-2")
        assert msg["jsonrpc"] == "2.0"
        assert msg["error"]["code"] == -32601
        assert msg["error"]["message"] == "Method not found"
        assert msg["id"] == "req-2"

    def test_create_error_no_id(self):
        msg = create_error(PARSE_ERROR, "Parse error")
        assert "id" not in msg

    def test_create_error_with_data(self):
        msg = create_error(-32602, "Bad params", data={"field": "target"})
        assert msg["error"]["data"]["field"] == "target"

    def test_create_notification(self):
        msg = create_notification("notifications/initialized")
        assert msg["jsonrpc"] == "2.0"
        assert msg["method"] == "notifications/initialized"
        assert "id" not in msg

    def test_create_notification_with_params(self):
        msg = create_notification("notifications/test", {"key": "val"})
        assert msg["params"]["key"] == "val"


class TestStdioTransport(object):
    """Test stdio transport with Content-Length framing."""

    def test_send_and_receive(self):
        send_buf = StringIO()
        recv_buf = StringIO()
        body = json.dumps({"jsonrpc": "2.0", "id": 1, "result": "ok"},
                          ensure_ascii=False)
        raw = body.encode("utf-8")
        header = "Content-Length: %d\r\n\r\n" % len(raw)
        recv_buf.write(header)
        recv_buf.write(body)
        recv_buf.seek(0)

        transport = StdioTransport()
        transport.stdout = send_buf
        transport.stdin = recv_buf

        msg = transport.read_message()
        assert msg is not None
        assert msg["id"] == 1
        assert msg["result"] == "ok"

    def test_read_eof(self):
        recv_buf = StringIO()
        transport = StdioTransport()
        transport.stdin = recv_buf
        msg = transport.read_message()
        assert msg is None

    def test_send_message(self):
        send_buf = StringIO()
        transport = StdioTransport()
        transport.stdout = send_buf

        transport.send_message({"jsonrpc": "2.0", "result": "ok", "id": 1})
        output = send_buf.getvalue()
        assert "Content-Length:" in output
        assert '"jsonrpc": "2.0"' in output
        assert '"result": "ok"' in output

    def test_log(self):
        err_buf = StringIO()
        transport = StdioTransport()
        transport.stderr = err_buf
        transport.log("test message")
        output = err_buf.getvalue()
        assert "[MCP] test message" in output


class TestMCPError(object):
    """Test MCPError exception."""

    def test_basic_error(self):
        e = MCPError(-32601, "Not found")
        assert e.code == -32601
        assert e.message == "Not found"
        assert e.to_dict()["code"] == -32601

    def test_error_with_data(self):
        e = MCPError(-32602, "Bad", data={"x": 1})
        assert e.to_dict()["data"]["x"] == 1


class TestServerHandshake(object):
    """Test MCP server initialize handshake."""

    def test_initialize_response(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()

        result = server._handle_initialize({}, "init-1")
        # _handle_initialize sends response, returns None

    def test_list_tools(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        assert len(server._tools) == 3
        names = [t["name"] for t in server._tools]
        assert "quality_check" in names
        assert "generate_content" in names
        assert "build_knowledge" in names

    def test_tool_input_schemas(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        for tool in server._tools:
            assert "inputSchema" in tool
            assert "type" in tool["inputSchema"]
            assert tool["inputSchema"]["type"] == "object"


class TestToolExecution(object):
    """Test MCP tool execution logic."""

    def test_unknown_tool(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        try:
            server._execute_tool("nonexistent", {})
            assert False, "Should raise ValueError"
        except ValueError as e:
            assert "Unknown tool" in str(e)

    def test_generate_content_missing_template(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        try:
            server._execute_tool("generate_content", {"template": ""})
            assert False, "Should raise ValueError"
        except ValueError:
            pass

    def test_build_knowledge_missing_input(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        try:
            server._execute_tool("build_knowledge",
                                 {"input_dir": "/nonexistent_xyz_123",
                                  "name": "test"})
            assert False, "Should raise ValueError"
        except ValueError:
            pass

    def test_quality_check_nonexistent_target(self):
        from mcp.server import DocSolutionServer
        server = DocSolutionServer()
        try:
            server._execute_tool("quality_check",
                                 {"target": "/nonexistent_xyz_123"})
            assert False, "Should raise ValueError"
        except ValueError:
            pass
