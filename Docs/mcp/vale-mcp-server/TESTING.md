# Test Scripts for Vale MCP Server

This directory contains test scripts for running unit tests.

## Quick Start

```powershell
# Run all tests
.\run-tests.ps1

# Run with verbose output
.\run-tests.ps1 -Verbose

# Run specific package
go test -v ./cmd/server/...
go test -v ./internal/handlers/...
```

## Test Coverage

| Package | Tests | Description |
|---------|-------|-------------|
| `cmd/server` | 9 | Middleware, API Key auth, rate limiting |
| `internal/handlers` | 21 | Path validation, regex safety, tools |

## Test Results

```
=== RUN   TestAPIKeyAuth_ValidKey                     PASS
=== RUN   TestAPIKeyAuth_InvalidKey                   PASS
=== RUN   TestAPIKeyAuth_MissingKey                   PASS
=== RUN   TestAPIKeyAuth_NoAuthRequired               PASS
=== RUN   TestAPIKeyAuth_KeyFromQuery                 PASS
=== RUN   TestAPIKeyAuth_RateLimitExceeded            PASS
=== RUN   TestSanitizeLogData_Truncation              PASS
=== RUN   TestSanitizeLogData_ShortData               PASS

=== RUN   TestValidatePath_ValidPath                  PASS
=== RUN   TestValidatePath_RelativePath               PASS
=== RUN   TestValidatePath_PathTraversal_ParentDirectory PASS
=== RUN   TestValidatePath_PathTraversal_Encoded      PASS
=== RUN   TestValidatePath_NonExistentPath            PASS
=== RUN   TestValidatePath_SymlinkOutside             SKIP (Windows)
=== RUN   TestValidatePath_SymlinkInside              PASS
=== RUN   TestValidatePath_EmptyPath                  PASS
=== RUN   TestValidatePath_ReservedCharacters         PASS
=== RUN   TestValidatePath_WindowsDriveLetter         PASS
=== RUN   TestValidatePath_SameAsAllowedDir           PASS

=== RUN   TestSafeRegexCompile_ValidPattern           PASS
=== RUN   TestSafeRegexCompile_TooLong                PASS
=== RUN   TestSafeRegexCompile_DangerousPattern_NonCapturing PASS
=== RUN   TestSafeRegexCompile_DangerousPattern_Lookahead PASS
=== RUN   TestSafeRegexCompile_DangerousPattern_Lookbehind PASS
=== RUN   TestSafeRegexCompile_DangerousPattern_GreedyStar PASS
=== RUN   TestSafeRegexCompile_InvalidChars           PASS
=== RUN   TestSafeRegexCompile_ExcessiveBacktracking  PASS
=== RUN   TestSafeRegexCompile_SimplePattern          PASS
=== RUN   TestRegisterAllTools                        PASS
=== RUN   TestValeStatusTool_WithoutVale              PASS
=== RUN   TestApplyInstructions_Simplify               PASS
=== RUN   TestApplyInstructions_Passive               PASS

Total: 30 tests, 29 passed, 1 skipped
```

## Running Tests in CI/CD

```bash
# GitHub Actions example
- name: Run tests
  working-directory: Docs/mcp/vale-mcp-server
  run: go test -v -race -coverprofile=coverage.out ./...

# Upload coverage
- name: Upload coverage
  uses: actions/upload-artifact@v4
  with:
    name: coverage
    path: Docs/mcp/vale-mcp-server/coverage.out
```

## Notes

- Symlink tests are skipped on Windows (require admin/developer mode)
- Some regex patterns show warnings but are allowed for flexibility
- Tests use temporary directories and clean up after themselves