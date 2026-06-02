---
audience: ai-agent
priority: high
purpose: Complete methodology for AI Agent to construct knowledge base content from customer inputs
category: guide
last-updated: 2026-06-03
---

# Knowledge Base Construction Guide

> AI Agent: This is your step-by-step methodology for building knowledge base content from customer input materials.

---

## Overview

The `build-kb` CLI command creates the **directory structure and index** of a knowledge base. But the **semantic content** (terminology rules, Vale YAML rules, glossary entries, checklist items) must be authored by you, the AI Agent, using the methodology in this guide.

### What This Guide Covers

| Topic | Section |
|-------|---------|
| Input format processing (md/xlsx/csv/json/yaml/d.ts/...) | [Input Format Handling](#input-format-handling) |
| Complete 5-step construction workflow | [Construction Workflow](#construction-workflow) |
| Term and abbreviation extraction | [Terminology Extraction](#step-2-terminology-extraction) |
| Vale rule YAML generation (all 9 types) | [Vale Rule Generation](#step-3-vale-rule-generation) |
| Test standard to checklist conversion | [Test Standard Conversion](#step-4-test-standard-conversion) |
| Registration with build-kb | [Registration](#step-5-registration-with-build-kb) |
| Full example | [End-to-End Example](#end-to-end-example) |

---

## Input Format Handling

Customer input materials come in various formats. This table tells you how to process each one:

| Format | Extension | Processing Strategy | What to Extract |
|--------|-----------|-------------------|-----------------|
| Markdown | `.md` | Parse headings, code blocks, paragraphs, tables, lists | Document structure, terminology, style patterns, API signatures |
| Excel | `.xlsx` `.xls` | Read cell content as text; read per-sheet, per-column | Tables are often parameter specs, error code lists, terminology pairs (Col A=term, Col B=definition), test cases |
| CSV | `.csv` | Parse rows and columns | Similar to xlsx: parameter tables, term lists, test case matrices |
| JSON | `.json` | Flatten nested structure, key names are terms | API schemas, config specs, enum values, property names |
| YAML | `.yaml` `.yml` | Parse as structured data, key names are terms | Config specs, rule definitions, enum values |
| TypeScript | `.ts` `.d.ts` | Extract interface/type/enum names, function signatures, parameter names, comments | Type definitions, API interfaces, enum literals, JSDoc comments (contain term descriptions) |
| Python | `.py` | Extract function signatures, class names, docstrings | API definitions, parameter names, docstring-style specs |
| Jinja2 | `.j2` | Parse template variables `{{ var }}` and `{% %}` blocks | Template structure, variable naming conventions |
| Plain text | `.txt` | Read line by line | Simple specs, lists |

### Excel/CSV Processing Rules

When processing spreadsheets, apply these heuristics:

```yaml
Sheet naming:
  - "术语表" / "术语" / "Glossary" / "Terminology": term definitions
  - "参数" / "Parameters" / "API" / "接口": API parameter specs
  - "错误码" / "Error Codes": error code definitions
  - "测试用例" / "Test Cases": test case specifications
  - "模板" / "Templates": template specifications

Column detection:
  - Col A = term/name, Col B = definition/description (by convention)
  - Headers: look for first row with known keywords (term, name, 名称, 参数名, etc.)
  - Skip rows with empty key columns
```

---

## Construction Workflow

```
+-----------+     +-----------+     +-----------+     +-----------+     +-----------+
| Step 1    | --> | Step 2    | --> | Step 3    | --> | Step 4    | --> | Step 5    |
| Structure |     | Termino-  |     | Vale      |     | Test Std  |     | Register  |
| Analysis  |     | logy Extr |     | Rules Gen |     | Conversion|     | with      |
+-----------+     +-----------+     +-----------+     +-----------+     +-----------+
     |                 |                 |                 |                 |
     v                 v                 v                 v                 v
 style-profile     glossary/         rules/vale/        checklist/        config.yaml
 (manual)          terms.yaml        styles/*.yml       *.yaml            (via build-kb)
```

---

## Step 1: Document Structure Analysis

### What to Analyze

Analyze the customer's `.md` documents and record:

```yaml
# Questions to answer:
- What heading level pattern do they use? (H1 > H2 > H2, or H1 > H3?)
- What is the typical document template? (sections in what order?)
- How are code blocks annotated? (```typescript, ```java, or no annotation?)
- What paragraph length is typical? (short bullets or long prose?)
- What tables are used? (parameter tables, comparison tables?)
- Are there required sections that every document must have?
```

### Output: Update style-profile.yaml

Edit `knowledge/meta/style-profile.yaml` with your findings:

```yaml
file_count: 15
analyzed_count: 15
heading_stats:
  average_per_file: 8.3
  level_distribution:
    1: 15
    2: 42
    3: 58
    4: 10
  # Manual additions below:
  typical_pattern: "H1 > H2 > H3"        # Most common heading path
  has_intro_section: true                  # Docs typically start with an intro
has_required_sections:                     # Sections every doc must have
  - "API Reference"
  - "Parameters"
  - "Error Codes"

paragraph_stats:
  long_paragraph_ratio: 12.5
  typical_style: "bullet-point-heavy"      # Or "prose-heavy"

code_block_patterns:
  - language: "typescript"                 # Most common code block language
    frequency: 80%
  - language: "java"
    frequency: 15%

table_patterns:
  - style: "parameter-table"               # Name | Type | Required | Description
    columns: ["name", "type", "required", "description"]
```

### Pattern Detection Guide

| Observation | Likely Convention |
|-------------|-------------------|
| 80%+ code blocks use ` ```typescript ` | Require language annotation |
| All docs have "API Reference" + "Error Codes" | Mark as required sections |
| "page" and "Page" used interchangeably | Add terminology consistency rule |
| H1 > H2 > H4 (skipping H3) | Possibly intentional; or flag as warning |
| Bullet points avg 50 chars each | Acceptable; no paragraph length issue |

---

## Step 2: Terminology Extraction

### What to Look For

Extract terminology from ALL input formats, not just `.md`:

| Source | Where to Look | What to Extract |
|--------|---------------|-----------------|
| `.md` docs | Repeated terms, headings, code comments | Domain terms, API names, concepts |
| `.ts` / `.d.ts` | Interface names, type names, enum values, JSDoc `@param` | Type names, enum literals, parameter names |
| `.xlsx` terminology sheets | Column A/B pairs | Authoritative term definitions |
| `.json` / `.yaml` configs | Key names, enum arrays, description fields | Config terms, allowed values |
| `.csv` term lists | Direct term/definition pairs | Glossary entries |
| Template `.j2` | Template variable names | Template parameter naming conventions |

### Output: glossary/terms.yaml

```yaml
# Standard terms extracted from customer docs
startAbility:
  zh: "启动Ability"
  en: "startAbility"
  category: "api"
  notes: "Main entry point for feature activation"

FA:
  zh: "功能适配"
  en: "Feature Adaptation"
  category: "abbreviation"
  preferred: true                       # Use this over full form in docs

page:
  zh: "页面"
  en: "page"
  category: "common"
  alternatives:                          # Terms to avoid
    - "Page"
    - "界面"

# Extracted from .d.ts enum values
AbilityType:
  zh: "Ability类型"
  en: "AbilityType"
  category: "type"
  values:                                # Enum literal values
    - "FEATURE"
    - "PAGE"
    - "SERVICE"
```

### Output: glossary/abbreviations.yaml

```yaml
# Abbreviations found across customer materials
FA: "Feature Adaptation"
UI: "User Interface"
API: "Application Programming Interface"
SDK: "Software Development Kit"
MCP: "Model Context Protocol"
FA: "Feature Adaptation"
```

### Extraction Heuristics

```yaml
Heuristic 1 - Repeated capitalization pattern:
  If: A word is consistently ALLCAPS across multiple docs
  Then: It is likely an abbreviation; add to abbreviations.yaml

Heuristic 2 - Code-to-text mismatch:
  If: Code uses "startAbility" but docs use "start ability" or "start_ability"
  Then: Add terminology rule enforcing "startAbility"

Heuristic 3 - Term table from spreadsheet:
  If: xlsx has columns [术语, 说明] or [Term, Definition]
  Then: Each row is a glossary entry

Heuristic 4 - Type/interface from .d.ts:
  If: interface/type name appears in JSDoc @param
  Then: It is a domain type worth adding to glossary
```

---

## Step 3: Vale Rule Generation

### Rule Type Decision Tree

```
What kind of check do you need?
  |
  +-- "Don't use word X" → existence
  +-- "Use X, not Y"     → substitution
  +-- "Max N times"      → occurrence
  +-- "No double words"  → repetition
  +-- "Use one style"    → consistency
  +-- "If X then Y"      → conditional
  +-- "Must be CAPS"     → capitalization
  +-- "Custom dict"      → spelling
  +-- "Readability"      → metric
```

### Rule Type Templates

#### existence — Check if certain terms exist

Use when: You want to forbid or require specific words.

```yaml
# Scenario: 禁止使用"please"、"easy"等非技术用语
# File: rules/vale/styles/Custom/NoPlease.yml
extends: existence
message: "避免使用 '%s'，使用更精确的技术描述"
level: warning
ignorecase: true
scope: text
tokens:
  - 'please'
  - 'easily'
  - 'simply'
  - 'just'
```

#### substitution — Replace incorrect terms

Use when: Customer has a standardized term that must be used consistently.

```yaml
# Scenario: "拉起" → "启动", "page" → "页面"
# File: rules/vale/styles/Custom/TermSubstitution.yml
extends: substitution
message: "建议使用 '%s' 代替 '%s'"
level: error
ignorecase: true
swap:
  "拉起": "启动"
  "start up": "startUp"
  "page": "页面"
  "点击": "单击"
```

#### occurrence — Limit term frequency

Use when: A term should appear at most N times per page.

```yaml
# Scenario: "注意" 每页不超过 3 次
# File: rules/vale/styles/Custom/NoteOccurrence.yml
extends: occurrence
message: "本页出现 '%s' %d 次，建议不超过 3 次"
level: suggestion
scope: paragraph
max: 3
tokens:
  - '注意'
  - 'Note:'
```

#### repetition — Detect repeated words

Use when: You need to catch accidental word duplication.

```yaml
# Scenario: 检测重复词 "the the"
# File: rules/vale/styles/Custom/RepeatedWords.yml
extends: repetition
message: "'%s' 重复出现"
level: warning
ignorecase: true
tokens:
  - 'the'
  - 'a'
  - 'an'
  - 'to'
  - 'is'
  - 'in'
  - 'on'
  - 'at'
```

#### consistency — Enforce consistent usage

Use when: Two forms of the same concept are used interchangeably.

```yaml
# Scenario: "e-mail" 与 "email" 必须统一
# File: rules/vale/styles/Custom/EmailConsistency.yml
extends: consistency
message: "术语不一致: 同时使用了 '%s' 和 '%s'"
level: warning
ignorecase: true
either:
  - 'e-mail'
  - 'email'
```

#### conditional — If X then check Y

Use when: Using one term implies another should also be used.

```yaml
# Scenario: 如果出现 "click"，也要检查是否有用户操作指导
# File: rules/vale/styles/Custom/ClickConditional.yml
extends: conditional
message: "使用了 'click'，建议补充完整的用户操作路径"
level: suggestion
ignorecase: true
first: '\bclick\b'
second: '(click|tap|select|choose|press)'
```

#### capitalization — Check case patterns

Use when: Product names or terms have specific capitalization rules.

```yaml
# Scenario: "startAbility" not "startability" or "StartAbility"
# File: rules/vale/styles/Custom/CamelCaseTerms.yml
extends: capitalization
message: "'%s' 应使用小写驼峰"
level: error
ignorecase: false
match: $sentence
style: camelCase
indicators:
  - 'startAbility'
  - 'onForeground'
  - 'createModule'
```

#### spelling — Custom spell checking

Use when: Domain-specific terms are flagged as spelling errors by Vale's default dictionary.

```yaml
# Scenario: 添加行业术语到 Vale 拼写检查
# This is NOT a YAML rule file. Create a plain text word list instead.
# File: rules/vale/styles/Custom/vocab.txt
startAbility
AbilityType
onForeground
Lifecycle
HarmonyOS
```

Then reference it in `.vale.ini`:
```ini
[*.md]
Vale.Spelling = YES
Vale.Spelling.vocab = Custom
```

#### metric — Readability metrics

Use when: Documents must meet a minimum readability score.

```yaml
# Scenario: 要求文档 Flesch 阅读分数 >= 60
# File: rules/vale/styles/DocsStyle/Readability.yml
extends: metric
message: "Flesch 阅读分数 %s (建议 >= 60)"
level: suggestion
metrics:
  - FleschReadingEase
  - FleschKincaidGrade
score: 60
```

### How to Choose Severity

| Severity | When to Use |
|----------|-------------|
| `error` | Terminology that must be exact (API names, type names) |
| `warning` | Style conventions that should be followed |
| `suggestion` | Best practices, readability improvements |

### How to Choose Scope

| Scope | Applies To | Example |
|-------|------------|---------|
| `text` | All text content | General terminology |
| `heading` | Heading lines only | Heading format rules |
| `code` | Code blocks only | Code style rules |
| `table` | Table cells | Table format rules |
| `paragraph` | Paragraph-level | Paragraph repetition |
| `sentence` | Sentence-level | Sentence length |

---

## Step 3.5: Rule Testing

After creating a Vale rule, you must verify it works correctly. The system provides a **generic test harness** — `doc-solution test-rule` — that works with ANY Vale rule type regardless of what content it checks.

### How the Test Harness Works

```
test-rule --rule <rule.yml> --should-fail <fail.md> --should-pass <pass.md>
         |
         v
+-------+-------+    +-------------------+    +-------------------+
| Syntax Check   |--->| Positive Test     |--->| Negative Test     |
| (Vale accepts  |    | (should-fail doc  |    | (should-pass doc  |
|  the YAML?)    |    |  triggers alerts) |    |  has no alerts)   |
+---------------+    +-------------------+    +-------------------+
         |                     |                       |
         v                     v                       v
   valid_syntax          positive_test            negative_test
   = True/False          = True/False/None        = True/False/None
```

Three checks are performed:

| Check | What It Validates | Required Input |
|-------|-------------------|----------------|
| Syntax | Vale accepts the YAML without error | Rule file only |
| Positive | Violations are correctly detected | Rule + should-fail doc |
| Negative | No false positives on clean content | Rule + should-pass doc |

**This is generic** — the test harness does not know or care what the rule checks. It only verifies that:
- Vale can parse the rule (syntax)
- The should-fail document produces alerts (positive)
- The should-pass document produces zero alerts (negative)

### CLI Reference

```bash
# Basic syntax validation (rule file only)
doc-solution test-rule --rule rules/vale/styles/Custom/TermSubstitution.yml

# Full test with positive + negative
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --should-pass ./tests/should-pass.md

# JSON output for programmatic consumption
doc-solution test-rule \
    --rule rules/vale/styles/Custom/TermSubstitution.yml \
    --should-fail ./tests/should-fail.md \
    --output json
```

### Exit Codes

| Exit Code | Meaning |
|-----------|---------|
| 0 | PASS — all tests passed |
| 1 | FAIL — positive or negative test failed |
| 1 | SYNTAX_ERROR — rule file has syntax errors |

### How to Create Test Documents

For each rule you create, you must also create two test documents:

**should-fail.md** — Contains content that SHOULD trigger the rule.

```markdown
# Test - Should Fail

This document intentionally uses incorrect terminology.
The api should be capitalized to API.
The sdk should be capitalized to SDK.
```

**should-pass.md** — Contains content that should NOT trigger the rule.

```markdown
# Test - Should Pass

This document uses correct terminology.
The API is properly capitalized.
The SDK is properly capitalized.
```

### Test Document Construction Rules

When creating test documents, follow these principles:

```yaml
Rule: "A Vale rule of ANY type (existence, substitution, etc.)"

should-fail.md construction:
  1. Include at least one instance that clearly violates the rule
  2. Make the violation obvious (e.g., exact wrong term)
  3. Surround with normal text to confirm scope detection works

should-pass.md construction:
  1. Include the correct/canonical form of everything the rule checks
  2. Add near-matches to verify no false positives
     (e.g., rule checks for "api" -> should-pass includes "API" AND "APIs")
  3. Include minimal surrounding context

Edge cases to cover:
  - Case sensitivity: If rule has ignorecase: true, test both forms
  - Partial matches: "API_KEY" should not trigger a rule about "API"
  - Code blocks: Depending on scope, code blocks may be excluded
  - Headings: If scope excludes headings, verify no heading alerts
```

### Complete Example

**Rule file** (`rules/vale/styles/Custom/ProductName.yml`):
```yaml
extends: substitution
message: "使用 '%s' 替代 '%s'"
level: error
ignorecase: true
swap:
  "鸿蒙": "HarmonyOS"
  "harmonyos": "HarmonyOS"
```

**should-fail.md** (`tests/ProductName/should-fail.md`):
```markdown
# Test - Should Fail

This guide covers development on 鸿蒙.
The harmonyos SDK provides the APIs.
```

**should-pass.md** (`tests/ProductName/should-pass.md`):
```markdown
# Test - Should Pass

This guide covers development on HarmonyOS.
The HarmonyOS SDK provides the APIs.
```

**Run the test:**
```bash
doc-solution test-rule \
    --rule rules/vale/styles/Custom/ProductName.yml \
    --should-fail tests/ProductName/should-fail.md \
    --should-pass tests/ProductName/should-pass.md
```

**Expected output:**
```
Rule:    Custom.ProductName
File:    rules/vale/styles/Custom/ProductName.yml
Summary: PASS

[Syntax]
  Valid:  YES

[Positive Test]
  Result: PASS — rule caught violations
  Alert:  [error] 使用 'HarmonyOS' 替代 '鸿蒙'
  Alert:  [error] 使用 'HarmonyOS' 替代 'harmonyos'

[Negative Test]
  Result: PASS — no false positives
```

### Integration into Workflow

After creating rules (Step 3) and before processing test standards (Step 4):

```
Step 3: Create Vale rule YAML files
   |
   v
Step 3.5: Test each rule with test-rule
   |  - Create should-fail.md and should-pass.md
   |  - Run test-rule for each rule
   |  - Fix rules until all pass
   |  - Include test files in customer-inputs/ for reproducibility
   v
Step 4: Test standard conversion
```

### Repeatable Test Suite

For maintainability, place test files alongside your rules:

```
customer-inputs/
  |-- rules/vale/styles/Custom/
  |   |-- ProductName.yml
  |   +-- TermSubstitution.yml
  |-- tests/
  |   |-- ProductName/
  |   |   |-- should-fail.md
  |   |   +-- should-pass.md
  |   +-- TermSubstitution/
  |       |-- should-fail.md
  |       +-- should-pass.md
  +-- ...
```

This allows re-running all rule tests whenever rules are modified:

```bash
for rule in rules/vale/styles/Custom/*.yml; do
  name=$(basename "$rule" .yml)
  fail="tests/$name/should-fail.md"
  pass="tests/$name/should-pass.md"
  if [ -f "$fail" ] && [ -f "$pass" ]; then
    doc-solution test-rule --rule "$rule" --should-fail "$fail" --should-pass "$pass"
  fi
done
```

---

## Step 4: Test Standard Conversion

### Input Types and Conversion Strategies

| Input Format | Typical Content | Conversion to Checklist |
|-------------|----------------|------------------------|
| `.xlsx` test case sheets | Test case ID, steps, expected result, precondition | Each test case → checklist item (based on scenario) |
| `.md` test specifications | Test scenarios, environment setup, acceptance criteria | Acceptance criteria → check items |
| `.yaml`/.json test configs | Test parameters, allowed values, boundary values | Validation rules → check items |
| `.csv` test matrices | Combination of parameters and expected outputs | Parameter rules → check items |
| `.d.ts` type definitions | Interface shapes, optional/required fields, enum values | Required fields → check items |

### Conversion Methodology

For each test standard input, follow this process:

```yaml
Step 1: Identify test scope
  - What feature/module is being tested?
  - What is the input format? (xlsx table? md scenario list?)

Step 2: Extract check points
  - Preconditions → "doc must mention prerequisites"
  - Steps → "doc must cover all steps in correct order"
  - Expected results → "doc must state expected outcome"
  - Edge cases → "doc must cover error scenarios"
  - Boundary values → "doc must document parameter limits"

Step 3: Classify check type
  - "auto": can be automated (Vale rule or built-in check)
  - "ai-review": needs AI Agent review (subjective)
  - "manual": needs human review (context-dependent)

Step 4: Write to checklist YAML
```

### Example: Converting a Test Spec

**Input (`test-cases.xlsx`):**

| Test Case | Scenario | Steps | Expected |
|-----------|----------|-------|----------|
| TC-001 | Start ability | 1. Call startAbility() 2. Verify result | Ability started |
| TC-002 | Start with null param | Call startAbility(null) | Return error code 400 |
| TC-003 | Start in background | Call startAbility() when app is backgrounded | Queue and start when foreground |

**Output (`checklist/quality-checklist.yaml`):**

```yaml
checklist:
  - id: "tc-001-coverage"
    name: "正常启动场景 - 文档必须包含 startAbility 的基本调用步骤"
    source: "TC-001"
    type: "auto"
    severity: "error"

  - id: "tc-002-coverage"
    name: "异常参数场景 - 文档必须说明空参数的处理方式"
    source: "TC-002"
    type: "ai-review"
    severity: "warning"

  - id: "tc-003-coverage"
    name: "后台启动场景 - 文档必须说明后台状态下的行为"
    source: "TC-003"
    type: "ai-review"
    severity: "suggestion"

  - id: "param-boundary"
    name: "参数边界值测试 - 每个参数需注明取值范围"
    source: "extracted-from-param-tables"
    type: "auto"
    severity: "error"
```

### Example: Converting Type Definitions (.d.ts)

**Input (`api.d.ts`):**

```typescript
interface StartAbilityOptions {
  abilityName: string;       // Required
  startMode?: StartMode;     // Optional
  timeout: number;           // Required, ms
}

enum StartMode {
  FOREGROUND = "foreground",
  BACKGROUND = "background",
}
```

**Output:**

```yaml
# checklist/review-checklist.yaml additions:
  - id: "dts-param-coverage"
    name: "所有必需参数(abilityName, timeout)必须在文档中列出"
    source: "api.d.ts:StartAbilityOptions"
    type: "auto"
    severity: "error"

  - id: "dts-enum-values"
    name: "枚举值(FOREGROUND/BACKGROUND)必须在文档中解释"
    source: "api.d.ts:StartMode"
    type: "ai-review"
    severity: "suggestion"
```

```yaml
# glossary/terms.yaml additions:
StartMode.FOREGROUND:
  zh: "前台启动"
  en: "FOREGROUND"
  category: "enum-value"
  source: "api.d.ts"
```

---

## Step 5: Registration with build-kb

After creating all content in Steps 1-4, register everything with `build-kb`:

```bash
# Step 5a: Place all files in a single input directory
customer-inputs/
  |-- rules/vale/styles/Custom/
  |   |-- NoPlease.yml            # (from Step 3)
  |   |-- TermSubstitution.yml    # (from Step 3)
  |   +-- ...
  |-- glossary/
  |   |-- terms.yaml              # (from Step 2)
  |   +-- abbreviations.yaml      # (from Step 2)
  |-- checklist/
  |   +-- quality-checklist.yaml  # (from Step 4)
  |-- templates/
  |   +-- api-ref/
  |       +-- template.md.j2      # (customer provided)
  +-- meta/
      +-- style-profile.yaml      # (from Step 1)

# Step 5b: Run build-kb to register
doc-solution build-kb --input ./customer-inputs/ --name "CustomerName" --force
```

### What build-kb Actually Does

| Action | What It Produces |
|--------|-----------------|
| Creates directories | `rules/vale/styles/DocsStyle/`, `rules/custom/`, etc. |
| Scans for `.md` files | style-profile.yaml (heading/paragraph stats) |
| Generates 2 default Vale rules | (can be overwritten by your custom files) |
| Scans for `.j2` files | Template entries in config.yaml |
| Generates config.yaml | Index of all KB resources |

**Your custom files (Vale rules, glossary, checklists) are preserved as-is.** The `build-kb` command simply copies them into the output structure and creates the index.

---

## End-to-End Example

### Customer Inputs Received

```
customer-inputs/
  |-- docs/
  |   |-- api-reference.md         # Main API doc
  |   +-- development-guide.md     # Dev guide
  |-- specs/
  |   |-- api-definitions.d.ts     # TypeScript type defs
  |   |-- test-cases.xlsx          # Test case matrix
  |   +-- terminology.xlsx         # Term definitions (Col A=term, Col B=desc)
  |-- templates/
  |   +-- api-ref/
  |       +-- template.md.j2       # Existing Jinja2 template
```

### AI Agent Construction Process

```yaml
Step 1 - Document Analysis:
  Read: api-reference.md, development-guide.md
  Observe:
    - Uses H1 > H2 > H3 pattern consistently
    - Code blocks always annotated ```typescript
    - Common terms: startAbility, onForeground, AbilityType
    - Required sections: "API Reference", "Parameters", "Error Codes"

Step 2 - Terminology Extraction:
  From api-definitions.d.ts:
    - Interface: StartAbilityOptions { abilityName, startMode, timeout }
    - Enum: StartMode { FOREGROUND, BACKGROUND }
  From terminology.xlsx:
    - FA → Feature Adaptation
    - MCP → Model Context Protocol
  Write: glossary/terms.yaml + abbreviations.yaml

Step 3 - Vale Rule Generation:
  Create rules:
    - Custom/TermSubstitution.yml  (enforce camelCase API names)
    - Custom/NoPlease.yml          (forbid informal language)
    - DocsStyle/HeadingHierarchy.yml (adjust if needed)

Step 3.5 - Rule Testing:
  For each rule, create test docs + run test-rule:
    - tests/TermSubstitution/should-fail.md + should-pass.md
    - tests/TermSubstitution/ -> doc-solution test-rule --rule ... (PASS)
    - tests/NoPlease/ -> doc-solution test-rule --rule ... (PASS)
  Include test docs in customer-inputs/tests/ for reproducibility

Step 4 - Test Standard Conversion:
  From test-cases.xlsx:
    - TC-001: normal start → auto check item
    - TC-002: null param → ai-review check item
  Write: checklist/quality-checklist.yaml

Step 5 - Registration:
  Run: doc-solution build-kb --input ./customer-inputs/ --name "Huawei-HarmonyOS" --force
  Verify: doc-solution check --target ./customer-inputs/docs/
```

---

## Appendix: Complete Vale Rule Reference

| Rule Type | YAML Key | Required Fields | Use Case |
|-----------|----------|-----------------|----------|
| existence | `extends: existence` | `tokens` | Forbid/require words |
| substitution | `extends: substitution` | `swap` (key: value pairs) | Term replacement |
| occurrence | `extends: occurrence` | `max`, `tokens` | Limit frequency |
| repetition | `extends: repetition` | `tokens` | Catch duplicates |
| consistency | `extends: consistency` | `either` (list of 2+) | Enforce one style |
| conditional | `extends: conditional` | `first`, `second` | X implies Y check |
| capitalization | `extends: capitalization` | `match`, `style`, `indicators` | Case enforcement |
| spelling | (vocab file, not YAML) | `.txt` word list | Custom dictionary |
| metric | `extends: metric` | `metrics`, `score` | Readability score |

### Common YAML Fields (All Rule Types)

| Field | Type | Description |
|-------|------|-------------|
| `message` | string | Error message shown to user. `%s` = matched token |
| `level` | enum | `error`, `warning`, `suggestion` |
| `scope` | enum | `text`, `heading`, `code`, `table`, `paragraph`, `sentence` |
| `ignorecase` | bool | Case-insensitive matching |
| `action` | object | Auto-fix configuration (name + params) |

---

## Appendix: Reference Guide for build-kb

```bash
# Build new KB
doc-solution build-kb --input ./customer-inputs/ --name "CustomerName"

# Rebuild (overwrite existing)
doc-solution build-kb --input ./customer-inputs/ --name "CustomerName" --force

# Custom output path
doc-solution build-kb --input ./customer-inputs/ --name "CustomerName" --output ./kb-custom/

# The --input directory structure should be:
# customer-inputs/
#   |-- docs/           (source .md files for style analysis)
#   |-- templates/      (.j2 Jinja2 templates)
#   |-- rules/          (optional: Vale YAML rules)
#   |-- glossary/       (optional: terms.yaml, abbreviations.yaml)
#   |-- checklist/      (optional: quality-checklists YAML)
#   +-- meta/           (optional: style-profile.yaml)
```
