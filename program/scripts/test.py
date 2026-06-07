# Test script for pipeline verification
# This runs automated checks on generated code

import sys
import os
from pathlib import Path


def check_file_exists(path: str) -> bool:
    """Check if a file exists."""
    exists = Path(path).exists()
    print(f"{'ok' if exists else 'not ok'} 1 - File exists: {path}")
    return exists


def check_file_contains(path: str, pattern: str) -> bool:
    """Check if a file contains a specific pattern."""
    if not Path(path).exists():
        print(f"not ok 1 - File not found: {path}")
        return False
    content = Path(path).read_text(encoding="utf-8")
    found = pattern in content
    print(f"{'ok' if found else 'not ok'} 1 - File '{path}' contains '{pattern}'")
    return found


def check_syntax_ets(path: str) -> bool:
    """Basic ArkTS syntax check."""
    if not Path(path).exists():
        return False
    content = Path(path).read_text(encoding="utf-8")
    issues = []
    if "@Entry" not in content and "@Component" not in content:
        issues.append("Missing @Entry or @Component decorator")
    if "build()" not in content:
        issues.append("Missing build() method")
    if issues:
        print(f"not ok 1 - Syntax issues in {path}: {'; '.join(issues)}")
        return False
    print(f"ok 1 - Syntax check passed: {path}")
    return True


def run_all_checks():
    """Run all verification checks."""
    checks = []
    output_dir = Path("output")

    if not output_dir.exists():
        print("not ok 1 - No output directory found")
        print("1..1")
        return False

    # Check ETS files
    ets_files = list(output_dir.rglob("*.ets"))
    for f in ets_files:
        checks.append(check_syntax_ets(str(f)))

    # Check C++ files for basic patterns
    cpp_files = list(output_dir.rglob("*.cpp")) + list(output_dir.rglob("*.h"))
    for f in cpp_files:
        checks.append(check_file_contains(str(f), "#include"))

    # Check config files
    config_files = list(output_dir.rglob("CMakeLists.txt"))
    for f in config_files:
        checks.append(check_file_contains(str(f), "cmake_minimum_required"))

    # Print summary
    total = len(checks)
    passed = sum(1 for c in checks if c)
    failed = total - passed
    print(f"\n# {passed}/{total} checks passed")

    return failed == 0


if __name__ == "__main__":
    success = run_all_checks()
    sys.exit(0 if success else 1)
