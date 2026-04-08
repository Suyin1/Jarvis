package checker

import (
	"context"
	"encoding/json"
	"os/exec"
	"strings"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

type CheckDocsArgs struct {
	FilePath string `json:"file_path"`
}

type ValeResult struct {
	Alerts []ValeAlert `json:"alerts"`
}

type ValeAlert struct {
	Check   string `json:"check"`
	Message string `json:"message"`
	Severity string `json:"severity"`
	Line    int    `json:"line"`
	Span    []int  `json:"span"`
}

func HandleCheckDocs(ctx context.Context, req *mcp.CallToolRequest, args any) (*mcp.CallToolResult, error) {
	checkArgs, ok := args.(map[string]any)
	if !ok {
		return mcp.NewToolResultText("Invalid arguments"), nil
	}

	filePath, ok := checkArgs["file_path"].(string)
	if !ok || filePath == "" {
		return mcp.NewToolResultText("file_path is required"), nil
	}

	cmd := exec.Command("vale", "--output=JSON", filePath)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return mcp.NewToolResultText("Vale check failed: " + err.Error()), nil
	}

	var result map[string]map[string][]ValeAlert
	if err := json.Unmarshal(output, &result); err != nil {
		return mcp.NewToolResultText("Failed to parse Vale output: " + err.Error()), nil
	}

	var alerts []string
	for file, checks := range result {
		if len(checks) == 0 {
			continue
		}
		alerts = append(alerts, "## Vale Check Report for "+file+"\n")
		for _, alert := range checks {
			severity := alert.Severity
			if severity == "" {
				severity = "warning"
			}
			alerts = append(alerts, "**Line "+strings.TrimSpace(string(rune(alert.Line)))+"** ["+severity+"] "+alert.Check+": "+alert.Message)
		}
	}

	if len(alerts) == 0 {
		return mcp.NewToolResultText("No issues found."), nil
	}

	return mcp.NewToolResultText(strings.Join(alerts, "\n")), nil
}

func HandleValeStatus(ctx context.Context, req *mcp.CallToolRequest, args any) (*mcp.CallToolResult, error) {
	cmd := exec.Command("vale", "--version")
	output, err := cmd.CombinedOutput()
	if err != nil {
		return mcp.NewToolResultText("Vale is not installed or not in PATH"), nil
	}
	return mcp.NewToolResultText("Vale is installed: " + strings.TrimSpace(string(output))), nil
}
