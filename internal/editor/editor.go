package editor

import (
	"context"
	"os"
	"regexp"
	"strings"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

type FixDocsArgs struct {
	FilePath    string `json:"file_path"`
	Instructions string `json:"instructions"`
	Pattern     string `json:"pattern,omitempty"`
	Replacement string `json:"replacement,omitempty"`
}

func HandleFixDocs(ctx context.Context, req *mcp.CallToolRequest, args any) (*mcp.CallToolResult, error) {
	fixArgs, ok := args.(map[string]any)
	if !ok {
		return mcp.NewToolResultText("Invalid arguments"), nil
	}

	filePath, ok := fixArgs["file_path"].(string)
	if !ok || filePath == "" {
		return mcp.NewToolResultText("file_path is required"), nil
	}

	instructions, _ := fixArgs["instructions"].(string)

	content, err := os.ReadFile(filePath)
	if err != nil {
		return mcp.NewToolResultText("Failed to read file: " + err.Error()), nil
	}

	modified := string(content)

	if pattern, ok := fixArgs["pattern"].(string); ok && pattern != "" {
		if replacement, ok := fixArgs["replacement"].(string); ok {
			re := regexp.MustCompile(pattern)
			count := re.ReplaceAllString(modified, replacement)
			if count != modified {
				modified = count
			}
		}
	}

	if instructions != "" {
		modified = applyInstructions(modified, instructions)
	}

	if err := os.WriteFile(filePath, []byte(modified), 0644); err != nil {
		return mcp.NewToolResultText("Failed to write file: " + err.Error()), nil
	}

	return mcp.NewToolResultText("File modified successfully. Changes applied based on: " + instructions), nil
}

func applyInstructions(content, instructions string) string {
	rules := map[string]string{
		"passive voice": `is|are|was|were|been|being\b`,
		"weasel words":  `\b(maybe|perhaps|probably|likely|possibly)\b`,
		"complex words": `\b(utilize|implement|facilitate)\b`,
	}

	for rule, pattern := range rules {
		if strings.Contains(strings.ToLower(instructions), rule) {
			re := regexp.MustCompile(`(?i)` + pattern)
			content = re.ReplaceAllStringFunc(content, func(match string) string {
				switch strings.ToLower(match) {
				case "utilize":
					return "use"
				case "implement":
					return "do"
				case "facilitate":
					return "help"
				default:
					return match
				}
			})
		}
	}

	return content
}
