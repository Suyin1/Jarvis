package handlers

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"regexp"
	"strings"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

func RegisterAllTools(server *mcp.Server, allowedDir string) {
	registerValeCheckTool(server)
	registerValeFixTool(server, allowedDir)
	registerValeStatusTool(server)
}

func registerValeCheckTool(server *mcp.Server) {
	tool := mcp.Tool{
		Name:        "check_docs",
		Description: "Check a document file for style and grammar issues using Vale.",
		InputSchema: mcp.InputSchema{
			Type: "object",
			Properties: map[string]mcp.Property{
				"file_path": {
					Type:        "string",
					Description: "The path to the document file to check",
				},
			},
			Required: []string{"file_path"},
		},
	}

	handler := func(args map[string]interface{}) (*mcp.CallToolResult, error) {
		filePath, ok := args["file_path"].(string)
		if !ok || filePath == "" {
			return mcp.ErrorResult("file_path is required"), nil
		}

		cmd := exec.Command("vale", "--output=JSON", filePath)
		output, err := cmd.CombinedOutput()
		if err != nil {
			return mcp.ErrorResult(fmt.Sprintf("Vale check failed: %v\nOutput: %s", err, string(output))), nil
		}

		var result map[string]map[string][]ValeAlert
		if err := json.Unmarshal(output, &result); err != nil {
			return mcp.ErrorResult(fmt.Sprintf("Failed to parse Vale output: %v", err)), nil
		}

		var alerts []string
		for file, checks := range result {
			if len(checks) == 0 {
				continue
			}
			alerts = append(alerts, fmt.Sprintf("## Vale Check Report for %s\n", file))
			for _, alert := range checks {
				severity := alert.Severity
				if severity == "" {
					severity = "warning"
				}
				alerts = append(alerts, fmt.Sprintf("**Line %d** [%s] %s: %s", alert.Line, severity, alert.Check, alert.Message))
			}
		}

		if len(alerts) == 0 {
			return mcp.SuccessResult("No issues found. Your document follows the style guidelines."), nil
		}

		return mcp.SuccessResult(strings.Join(alerts, "\n")), nil
	}

	server.Tools().Register(tool, handler)
}

func registerValeFixTool(server *mcp.Server, allowedDir string) {
	tool := mcp.Tool{
		Name:        "fix_docs",
		Description: "Apply automated fixes to a document based on Vale linting results or custom instructions.",
		InputSchema: mcp.InputSchema{
			Type: "object",
			Properties: map[string]mcp.Property{
				"file_path": {
					Type:        "string",
					Description: "The path to the document file to fix",
				},
				"instructions": {
					Type:        "string",
					Description: "Fix instructions (e.g., 'fix passive voice', 'use simpler words')",
				},
				"pattern": {
					Type:        "string",
					Description: "Regex pattern to find and replace",
				},
				"replacement": {
					Type:        "string",
					Description: "Replacement text for the pattern",
				},
			},
			Required: []string{"file_path"},
		},
	}

	handler := func(args map[string]interface{}) (*mcp.CallToolResult, error) {
		filePath, ok := args["file_path"].(string)
		if !ok || filePath == "" {
			return mcp.ErrorResult("file_path is required"), nil
		}

		content, err := os.ReadFile(filePath)
		if err != nil {
			return mcp.ErrorResult(fmt.Sprintf("Failed to read file: %v", err)), nil
		}

		modified := string(content)

		if pattern, ok := args["pattern"].(string); ok && pattern != "" {
			if replacement, ok := args["replacement"].(string); ok {
				re := regexp.MustCompile(pattern)
				modified = re.ReplaceAllString(modified, replacement)
			}
		}

		if instructions, ok := args["instructions"].(string); ok && instructions != "" {
			modified = applyInstructions(modified, instructions)
		}

		if err := os.WriteFile(filePath, []byte(modified), 0644); err != nil {
			return mcp.ErrorResult(fmt.Sprintf("Failed to write file: %v", err)), nil
		}

		return mcp.SuccessResult(fmt.Sprintf("File modified successfully: %s", filePath)), nil
	}

	server.Tools().Register(tool, handler)
}

func registerValeStatusTool(server *mcp.Server) {
	tool := mcp.Tool{
		Name:        "vale_status",
		Description: "Check if Vale is properly installed and accessible.",
		InputSchema: mcp.InputSchema{
			Type:       "object",
			Properties: map[string]mcp.Property{},
		},
	}

	handler := func(args map[string]interface{}) (*mcp.CallToolResult, error) {
		cmd := exec.Command("vale", "--version")
		output, err := cmd.CombinedOutput()
		if err != nil {
			return mcp.ErrorResult("Vale is not installed or not in PATH"), nil
		}
		return mcp.SuccessResult("Vale is installed: " + strings.TrimSpace(string(output))), nil
	}

	server.Tools().Register(tool, handler)
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
		"utilize":          "use",
		"implement":         "do",
		"facilitate":       "help",
		"in order to":       "to",
		"due to the fact that": "because",
		"at this point in time": "now",
		"in the event that": "if",
		"has the ability to": "can",
	}

	if strings.Contains(instructions, "simple") || strings.Contains(instructions, "weasel") || strings.Contains(instructions, "complex") {
		for complex, simple := range simplifications {
			re := regexp.MustCompile(`(?i)\b` + regexp.QuoteMeta(complex) + `\b`)
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
