---
audience: customer
priority: high
purpose: Security and privacy proof for customer security reviewers
category: security
last-updated: 2026-06-03
---

# Security and Privacy Statement

> Audience: Customer decision-makers and security reviewers

---

## Network Isolation: Zero External Communication

All components of the Doc Solution System are designed to operate in **completely isolated, air-gapped environments**. No system component makes any network request, phones home, sends telemetry, or communicates with external servers.

### Component Network Audit

| Component | Network Access | Mechanism |
|-----------|---------------|-----------|
| Python Engine (parser/checker) | **None** | Pure local file I/O |
| Vale Binary (knowledge/vale.exe) | **None** | Reads files, applies local YAML rules, outputs to stdout |
| Jinja2 Templates | **None** | Local file-based template loading |
| CLI Tools | **None** | All operations are local filesystem |
| MCP Server | **None** | stdin/stdout communication only |
| Knowledge Base | **None** | Local YAML/INI configuration files |
| Configuration Files | **None** | Read from local filesystem |

### How Vale Ensures Offline Operation

Vale is a static analysis tool with the following architecture:

```
Input: .md files (local)
       .vale.ini (local)
       styles/*.yml (local)
       |
       v
Vale binary --> Reads files --> Applies rules --> Outputs to stdout
       |
       No DNS lookups
       No HTTP requests
       No telemetry
       No update checks
       No license validation
```

Key points:
- Vale has **no networking code** in its codebase
- All style rules are YAML files stored locally
- Built-in styles (Vale, Microsoft) are **compiled into the binary**, not downloaded
- The `--no-global` flag prevents loading any global configuration
- No external dependencies at runtime

## Data Flow

```
Customer's machine (isolated)
==============================
Source documents (local .md files)
       |
       v
Doc Solution System
  - Reads files
  - Applies rules
  - Generates reports
       |
       v
Output: Terminal / JSON file / MCP response
All output stays on the local machine
==============================
           |  No data ever leaves
           v
     Internet (not accessed)
```

## Frequently Asked Questions

### Q: Does Vale make any network requests?

**No.** Vale is a completely offline tool. It reads files from your local filesystem, applies rules from your local configuration, and outputs results to stdout. There is no networking functionality in the Vale binary.

### Q: Can I use this in an air-gapped environment?

**Yes.** This is the primary use case. All components run offline. The Vale binary is bundled in the project (`knowledge/vale.exe`) and requires no download, installation, or internet access.

### Q: Does the MCP Server expose any network ports?

**No.** The MCP Server communicates exclusively via stdin/stdout. There are no TCP ports, no HTTP endpoints, and no network sockets. Communication is between the AI Agent and the server process via standard pipes.

### Q: What about configuration files - do they reference external resources?

**No.** All configuration files (`.vale.ini`, `config.yaml`, style rules) reference only local paths. There is no URL, domain, or IP address in any configuration.

### Q: How can I verify there is no network activity?

```bash
# Option 1: Run with network monitoring
# Monitor connections while running:
python -m tools.cli check --target ./docs/

# Option 2: Use --no-global flag to ensure no global config is loaded
python -m tools.cli check --target ./docs/ --config knowledge/rules/vale/.vale.ini

# Option 3: Verify Vale itself has no network flags
vale --help  # No network-related flags exist
```

### Q: What about future updates?

System updates are delivered via git (self-hosted or mirror). Vale binary updates, if needed, are distributed as file replacements through the customer's standard software update process. No automatic updates, no phone-home.
