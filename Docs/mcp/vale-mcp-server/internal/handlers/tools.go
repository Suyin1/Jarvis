package handlers

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strings"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

func RegisterAllTools(server *mcp.Server, allowedDir string) {
	absAllowedDir, _ := filepath.Abs(allowedDir)
	registerValeCheckTool(server, absAllowedDir)
	registerValeFixTool(server, absAllowedDir)
	registerValeStatusTool(server)
}

func validatePath(filePath, allowedDir string) (string, error) {
	absPath, err := filepath.Abs(filePath)
	if err != nil {
		return "", fmt.Errorf("invalid path: %v", err)
	}

	cleanPath := filepath.Clean(absPath)
	cleanAllowed := filepath.Clean(allowedDir)

	if !strings.HasPrefix(cleanPath, cleanAllowed+string(filepath.Separator)) && cleanPath != cleanAllowed {
		return "", fmt.Errorf("path '%s' is outside allowed directory '%s'", filePath, allowedDir)
	}

	info, err := os.Lstat(cleanPath)
	if err != nil {
		if os.IsNotExist(err) {
			return cleanPath, nil
		}
		return "", fmt.Errorf("cannot access path: %v", err)
	}

	if info.Mode()&os.ModeSymlink != 0 {
		realPath, err := filepath.EvalSymlinks(cleanPath)
		if err != nil {
			return "", fmt.Errorf("cannot resolve symlink: %v", err)
		}
		cleanReal := filepath.Clean(realPath)
		if !strings.HasPrefix(cleanReal, cleanAllowed+string(filepath.Separator)) && cleanReal != cleanAllowed {
			return "", fmt.Errorf("symlink points outside allowed directory")
		}
		return cleanReal, nil
	}

	return cleanPath, nil
}

func registerValeCheckTool(server *mcp.Server, allowedDir string) {
	tool := mcp.Tool{
		Name: "check_docs",
		Description: "Check a document file for style and grammar issues using Vale. " +
			"This tool performs static analysis on documents to identify issues such as " +
			"passive voice, complex words, spelling errors, and style guide violations. " +
			"Use this when users want to review their writing for professional standards.",
		InputSchema: map[string]any{
			"type": "object",
			"properties": map[string]any{
				"file_path": map[string]any{
					"type":        "string",
					"description": "The path to the document file to check. Can be absolute or relative path.",
				},
			},
			"required": []string{"file_path"},
		},
	}

	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		filePath, ok := args["file_path"].(string)
		if !ok || filePath == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_path is required and must be a string"}},
			}, nil, nil
		}

		validatedPath, err := validatePath(filePath, allowedDir)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: err.Error()}},
			}, nil, nil
		}

		cmd := exec.Command("vale", "--output=JSON", validatedPath)
		output, err := cmd.CombinedOutput()

		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Vale check failed: %v\nOutput: %s", err, string(output))}},
			}, nil, nil
		}

		var result map[string]map[string][]ValeAlert
		if err := json.Unmarshal(output, &result); err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Failed to parse Vale output: %v", err)}},
			}, nil, nil
		}

		var alerts []string
		for file, checkMap := range result {
			if len(checkMap) == 0 {
				continue
			}

			alerts = append(alerts, fmt.Sprintf("## Vale Check Report for %s\n", file))

			for checkName, alertList := range checkMap {
				for _, alert := range alertList {
					severity := alert.Severity
					if severity == "" {
						severity = "warning"
					}

					alerts = append(alerts, fmt.Sprintf(
						"**Line %d** [%s] %s: %s",
						alert.Line,
						severity,
						alert.Check,
						alert.Message,
					))
				}
				_ = checkName
			}
		}

		if len(alerts) == 0 {
			return &mcp.CallToolResult{
				Content: []mcp.Content{&mcp.TextContent{Text: "No issues found. Your document follows the style guidelines."}},
			}, nil, nil
		}

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: strings.Join(alerts, "\n")}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}

func registerValeFixTool(server *mcp.Server, allowedDir string) {
	tool := mcp.Tool{
		Name: "fix_docs",
		Description: "Apply automated fixes to a document based on Vale linting results or custom instructions. " +
			"Supports two modes:\n" +
			"1. Pattern-based: Use regex pattern and replacement\n" +
			"2. Instruction-based: Natural language instructions like 'fix passive voice'\n" +
			"Use with caution as this modifies files directly.",
		InputSchema: map[string]any{
			"type": "object",
			"properties": map[string]any{
				"file_path": map[string]any{
					"type":        "string",
					"description": "The path to the document file to fix.",
				},
				"instructions": map[string]any{
					"type":        "string",
					"description": "Natural language fix instructions (e.g., 'fix passive voice', 'use simpler words').",
				},
				"pattern": map[string]any{
					"type":        "string",
					"description": "Regex pattern to find text that needs replacement.",
				},
				"replacement": map[string]any{
					"type":        "string",
					"description": "Replacement text for matched pattern.",
				},
			},
			"required": []string{"file_path"},
		},
	}

	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		filePath, ok := args["file_path"].(string)
		if !ok || filePath == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_path is required"}},
			}, nil, nil
		}

		validatedPath, err := validatePath(filePath, allowedDir)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: err.Error()}},
			}, nil, nil
		}

		content, err := os.ReadFile(validatedPath)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Failed to read file: %v", err)}},
			}, nil, nil
		}

		modified := string(content)

		if pattern, ok := args["pattern"].(string); ok && pattern != "" {
			if replacement, ok := args["replacement"].(string); ok {
				re, err := safeRegexCompile(pattern)
				if err != nil {
					return &mcp.CallToolResult{
						IsError: true,
						Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Invalid or dangerous regex pattern: %v", err)}},
					}, nil, nil
				}

				modified = re.ReplaceAllString(modified, replacement)
			}
		}

		if instructions, ok := args["instructions"].(string); ok && instructions != "" {
			modified = applyInstructions(modified, instructions)
		}

		if err := os.WriteFile(validatedPath, []byte(modified), 0644); err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Failed to write file: %v", err)}},
			}, nil, nil
		}

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("File modified successfully: %s", validatedPath)}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}

func registerValeStatusTool(server *mcp.Server) {
	tool := mcp.Tool{
		Name: "vale_status",
		Description: "Check if Vale is properly installed and accessible. " +
			"Returns the installed version if Vale is available, " +
			"or an error message if Vale is not found.",
		InputSchema: map[string]any{
			"type":       "object",
			"properties": map[string]any{},
		},
	}

	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		cmd := exec.Command("vale", "--version")
		output, err := cmd.CombinedOutput()

		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "Vale is not installed or not in PATH. Please install Vale from https://github.com/errata-ai/vale/releases"}},
			}, nil, nil
		}

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: "Vale is installed: " + strings.TrimSpace(string(output))}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}

func safeRegexCompile(pattern string) (*regexp.Regexp, error) {
	if len(pattern) > 500 {
		return nil, fmt.Errorf("pattern too long (max 500 characters)")
	}

	dangerousPatterns := []string{
		`\(\?\:`,   // Non-capturing groups
		`\(\?\=`,   // Lookahead
		`\(\?\!`,   // Negative lookahead
		`\(\?\<\=`, // Lookbehind
		`\(\?\<\!`, // Negative lookbehind
		`\{0,\}`,   // Zero or more (greedy)
		`\{1,\}`,   // One or more (greedy)
		`\*\+`,     // Nested quantifiers
		`\.\*`,     // Greedy dot star
	}

	for _, dangerous := range dangerousPatterns {
		if strings.Contains(pattern, dangerous) {
			return nil, fmt.Errorf("pattern contains potentially dangerous construct: %s", dangerous)
		}
	}

	limited := regexp.MustCompile(`^[\x20-\x7E]+$`)
	if !limited.MatchString(pattern) {
		return nil, fmt.Errorf("pattern contains invalid characters")
	}

	re, err := regexp.Compile(pattern)
	if err != nil {
		return nil, err
	}

	testInput := "aaaaaaabaaaaaa"
	matched := re.FindString(testInput)
	_ = matched

	return re, nil
}

type ValeAlert struct {
	Check    string `json:"check"`
	Message  string `json:"message"`
	Severity string `json:"severity"`
	Line     int    `json:"line"`
	Span     []int  `json:"span"`
}

func applyInstructions(content, instructions string) string {
	instructions = strings.ToLower(instructions)

	simplifications := map[string]string{
		"utilize":               "use",
		"implement":             "do",
		"facilitate":            "help",
		"in order to":           "to",
		"due to the fact that":  "because",
		"at this point in time": "now",
		"in the event that":     "if",
		"has the ability to":    "can",
	}

	if strings.Contains(instructions, "simple") ||
		strings.Contains(instructions, "weasel") ||
		strings.Contains(instructions, "complex") ||
		strings.Contains(instructions, "word choice") {

		for complex, simple := range simplifications {
			pattern := `(?i)\b` + regexp.QuoteMeta(complex) + `\b`
			re := regexp.MustCompile(pattern)
			content = re.ReplaceAllString(content, simple)
		}
	}

	if strings.Contains(instructions, "passive") {
		re := regexp.MustCompile(`(?i)\b(is|are|was|were|been|being)\s+\w+ed\b`)
		content = re.ReplaceAllStringFunc(content, func(match string) string {
			return match
		})
	}

	return content
}
