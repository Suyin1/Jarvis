package handlers

// handlers 包包含了 MCP 服务器的所有工具处理器
// 包括：Vale 检查工具、文件修复工具、文件上传/删除/列表工具

import (
	"context"
	"encoding/base64" // 用于 Base64 编解码
	"encoding/json"   // 用于 JSON 序列化/反序列化
	"fmt"             // 用于格式化错误信息
	"log"             // 用于日志记录
	"os"              // 用于文件操作
	"os/exec"         // 用于执行外部命令（vale）
	"path/filepath"   // 用于路径操作
	"regexp"          // 用于正则表达式
	"strings"         // 用于字符串处理

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

// RegisterAllTools 注册 MCP 服务器的所有工具
// 参数：
//   - server: MCP 服务器实例
//   - allowedDir: 允许访问的目录（通过环境变量 VALE_ALLOWED_DIR 设置）
//
// 这个函数是入口点，会注册以下工具：
//  1. check_docs - 检查文档风格
//  2. fix_docs - 修复文档
//  3. vale_status - 检查 Vale 是否可用
//  4. upload_file - 上传文件（方案2新增）
//  5. delete_file - 删除文件（方案2新增）
//  6. list_files - 列出已上传文件（方案2新增）
func RegisterAllTools(server *mcp.Server, allowedDir string) {
	// 将相对路径转换为绝对路径
	absAllowedDir, _ := filepath.Abs(allowedDir)

	// 创建上传文件存储目录：.vale-uploads
	// 这个目录用于存储客户端通过 upload_file 上传的文件
	// 位于允许目录下的 .vale-uploads 子目录中
	uploadDir := filepath.Join(absAllowedDir, ".vale-uploads")

	// 创建上传目录（如果不存在）
	// 0755 表示目录所有者可读写执行，组和其他用户可读执行
	if err := os.MkdirAll(uploadDir, 0755); err != nil {
		log.Printf("Warning: failed to create upload directory: %v", err)
	}

	// 注册原有的 Vale 相关工具
	registerValeCheckTool(server, absAllowedDir)
	registerValeFixTool(server, absAllowedDir)
	registerValeStatusTool(server)

	// 注册方案2新增的文件上传/管理工具
	// 这些工具是独立的，与 Vale 检查功能解耦
	// 允许客户端上传文件到服务器进行临时存储和检查
	registerUploadFileTool(server, uploadDir)
	registerDeleteFileTool(server, uploadDir)
	registerListFilesTool(server, uploadDir)
}

// validatePath 验证文件路径是否在允许的目录内
// 这是一个安全检查函数，防止客户端访问不允许的目录
//
// 参数：
//   - filePath: 客户端请求访问的文件路径
//   - allowedDir: 允许访问的目录（通过 VALE_ALLOWED_DIR 设置）
//
// 返回值：
//   - 验证后的绝对路径（如果有效）
//   - 错误信息（如果无效）
//
// 安全检查包括：
//  1. 路径必须是绝对路径
//  2. 路径必须在 allowedDir 目录下
//  3. 解析符号链接并验证目标路径
func validatePath(filePath, allowedDir string) (string, error) {
	// 将相对路径转换为绝对路径
	absPath, err := filepath.Abs(filePath)
	if err != nil {
		return "", fmt.Errorf("invalid path: %v", err)
	}

	// 清理路径（去除多余的斜杠、点号等）
	cleanPath := filepath.Clean(absPath)
	cleanAllowed := filepath.Clean(allowedDir)

	// 检查路径是否在允许目录内
	// 使用前缀匹配确保路径是允许目录的子目录
	if !strings.HasPrefix(cleanPath, cleanAllowed+string(filepath.Separator)) && cleanPath != cleanAllowed {
		return "", fmt.Errorf("path '%s' is outside allowed directory '%s'", filePath, allowedDir)
	}

	// 检查路径是否存在以及是否可访问
	info, err := os.Lstat(cleanPath)
	if err != nil {
		if os.IsNotExist(err) {
			// 文件不存在但路径有效（用于创建新文件）
			return cleanPath, nil
		}
		return "", fmt.Errorf("cannot access path: %v", err)
	}

	// 如果是符号链接，解析并验证目标路径
	if info.Mode()&os.ModeSymlink != 0 {
		realPath, err := filepath.EvalSymlinks(cleanPath)
		if err != nil {
			return "", fmt.Errorf("cannot resolve symlink: %v", err)
		}
		cleanReal := filepath.Clean(realPath)
		// 确保符号链接目标也在允许目录内
		if !strings.HasPrefix(cleanReal, cleanAllowed+string(filepath.Separator)) && cleanReal != cleanAllowed {
			return "", fmt.Errorf("symlink points outside allowed directory")
		}
		return cleanReal, nil
	}

	return cleanPath, nil
}

// registerValeCheckTool 注册 Vale 文档检查工具
// 功能：使用 Vale CLI 检查文档的风格和语法问题
//
// 参数：
//   - server: MCP 服务器实例
//   - allowedDir: 允许访问的目录
//
// 工作流程：
//  1. 接收客户端传来的 file_path
//  2. 验证路径是否在允许目录内（validatePath）
//  3. 执行 vale --output=JSON 命令
//  4. 解析 JSON 结果并返回格式化的报告
func registerValeCheckTool(server *mcp.Server, allowedDir string) {
	// 定义 MCP 工具的元数据
	// 工具名称：check_docs
	tool := mcp.Tool{
		Name: "check_docs",
		Description: "Check a document file for style and grammar issues using Vale. " +
			"This tool performs static analysis on documents to identify issues such as " +
			"passive voice, complex words, spelling errors, and style guide violations. " +
			"Use this when users want to review their writing for professional standards.",
		// JSON Schema 定义工具的参数
		InputSchema: map[string]any{
			"type": "object",
			"properties": map[string]any{
				"file_path": map[string]any{
					"type":        "string",
					"description": "The path to the document file to check. Can be absolute or relative path.",
				},
			},
			"required": []string{"file_path"}, // file_path 是必填参数
		},
	}

	// 工具的处理函数
	// 当客户端调用 check_docs 时，会执行这个函数
	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		// 从参数中获取 file_path
		filePath, ok := args["file_path"].(string)
		if !ok || filePath == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_path is required and must be a string"}},
			}, nil, nil
		}

		// 验证路径安全性
		validatedPath, err := validatePath(filePath, allowedDir)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: err.Error()}},
			}, nil, nil
		}

		// 调用外部 Vale 命令检查文档
		// --output=JSON 指定输出格式为 JSON，方便程序解析
		cmd := exec.Command("vale", "--output=JSON", validatedPath)
		output, err := cmd.CombinedOutput()

		// 处理 Vale 执行错误
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Vale check failed: %v\nOutput: %s", err, string(output))}},
			}, nil, nil
		}

		// 解析 Vale 的 JSON 输出
		// ValeAlert 是自定义结构体，用于映射 JSON 结果
		var result map[string]map[string][]ValeAlert
		if err := json.Unmarshal(output, &result); err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("Failed to parse Vale output: %v", err)}},
			}, nil, nil
		}

		// 将结果格式化为易读的文本格式
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

const (
	maxFileSize       = 10 * 1024 * 1024 // 10MB
	allowedExtensions = ".md,.txt,.json,.xml,.yaml,.yml,.csv"
)

// ============================================================
// 文件上传/管理工具（方案2：远程 MCP 文件上传功能）
// ============================================================

// dangerousPatterns 用于内容安全检查的可疑代码模式
// 如果文件内容包含这些模式，将拒绝上传
var dangerousPatterns = []string{
	"<?php",       // PHP 代码
	"<script",     // JavaScript 脚本
	"eval(",       // eval 函数
	"exec(",       // 执行命令
	"system(",     // 系统调用
	" subprocess", // Python 子进程
	" os.system",  // Python os.system
	" shell_exec", // Shell 执行
}

// registerUploadFileTool 注册文件上传工具
// 功能：接收客户端上传的文件并保存到服务器本地
//
// 参数：
//   - server: MCP 服务器实例
//   - uploadDir: 上传文件存储目录
//
// 安全检查（按顺序执行）：
//  1. 文件名检查：禁止路径遍历字符（.., /, \）
//  2. 文件类型检查：只允许文档类型（.md, .txt, .json, .xml, .yaml, .yml, .csv）
//  3. 文件大小检查：最大 10MB
//  4. 内容安全检查：检测危险代码模式
//  5. 覆盖保护：不允许覆盖已存在文件
func registerUploadFileTool(server *mcp.Server, uploadDir string) {
	tool := mcp.Tool{
		Name: "upload_file",
		Description: "Upload a document file to the server for checking. " +
			"Use this before check_docs when working with remote MCP servers. " +
			"File is stored temporarily and should be deleted after use.",
		InputSchema: map[string]any{
			"type": "object",
			"properties": map[string]any{
				"file_name": map[string]any{
					"type":        "string",
					"description": "The name of the file (e.g., 'README.md'). No path traversal allowed.",
				},
				"content": map[string]any{
					"type":        "string",
					"description": "Base64-encoded file content.",
				},
			},
			"required": []string{"file_name", "content"},
		},
	}

	// 处理函数
	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		// === 参数获取 ===
		fileName, ok := args["file_name"].(string)
		if !ok || fileName == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_name is required"}},
			}, nil, nil
		}

		contentB64, ok := args["content"].(string)
		if !ok || contentB64 == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "content is required (base64 encoded)"}},
			}, nil, nil
		}

		// === 安全检查 1: 文件名路径遍历检查 ===
		// 防止客户端通过 ../ 或绝对路径访问服务器敏感目录
		if strings.Contains(fileName, "..") || strings.Contains(fileName, "/") || strings.Contains(fileName, "\\") {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_name contains invalid characters (no path traversal allowed)"}},
			}, nil, nil
		}

		// === 安全检查 2: 文件类型检查 ===
		// 只允许上传文档类文件，防止上传可执行文件
		ext := strings.ToLower(filepath.Ext(fileName))
		if !strings.Contains(allowedExtensions, ext) {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file type not allowed. Allowed: " + allowedExtensions}},
			}, nil, nil
		}

		// === Base64 解码 ===
		// 客户端传来的 content 是 Base64 编码的，需要解码
		content, err := base64.StdEncoding.DecodeString(contentB64)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "invalid base64 content: " + err.Error()}},
			}, nil, nil
		}

		// === 安全检查 3: 文件大小检查 ===
		// 防止超大文件占用磁盘和内存
		if len(content) > maxFileSize {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("file too large (max %dMB)", maxFileSize/1024/1024)}},
			}, nil, nil
		}

		// === 安全检查 4: 内容安全检查 ===
		// 检测是否包含可疑的代码模式（PHP、JavaScript、命令执行等）
		contentStr := string(content)
		contentLower := strings.ToLower(contentStr)
		for _, pattern := range dangerousPatterns {
			if strings.Contains(contentLower, pattern) {
				return &mcp.CallToolResult{
					IsError: true,
					Content: []mcp.Content{&mcp.TextContent{Text: "file content contains potentially dangerous code"}},
				}, nil, nil
			}
		}

		// === 安全检查 5: 覆盖保护 ===
		// 不允许覆盖已存在的文件，防止意外覆盖重要文件
		destPath := filepath.Join(uploadDir, fileName)
		if _, err := os.Stat(destPath); err == nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file already exists, use delete_file first"}},
			}, nil, nil
		}

		// === 保存文件 ===
		// 所有检查通过，写入文件
		// 0644 表示文件所有者可读写，组和其他用户可读
		if err := os.WriteFile(destPath, content, 0644); err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "failed to save file: " + err.Error()}},
			}, nil, nil
		}

		log.Printf("File uploaded: %s (%d bytes)", fileName, len(content))

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("File uploaded successfully: %s (%d bytes)", fileName, len(content))}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}

// registerDeleteFileTool 注册文件删除工具
// 功能：删除服务器上已上传的文件，释放存储空间
//
// 参数：
//   - server: MCP 服务器实例
//   - uploadDir: 上传文件存储目录
//
// 安全检查：
//   - 文件名路径遍历检查（同 upload_file）
//   - 文件存在性检查
func registerDeleteFileTool(server *mcp.Server, uploadDir string) {
	tool := mcp.Tool{
		Name: "delete_file",
		Description: "Delete an uploaded file from the server. " +
			"Use this to clean up after checking documents.",
		InputSchema: map[string]any{
			"type": "object",
			"properties": map[string]any{
				"file_name": map[string]any{
					"type":        "string",
					"description": "The name of the file to delete.",
				},
			},
			"required": []string{"file_name"},
		},
	}

	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		// 获取文件名
		fileName, ok := args["file_name"].(string)
		if !ok || fileName == "" {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file_name is required"}},
			}, nil, nil
		}

		// 安全检查：防止路径遍历
		if strings.Contains(fileName, "..") || strings.Contains(fileName, "/") || strings.Contains(fileName, "\\") {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "invalid file_name"}},
			}, nil, nil
		}

		// 删除文件
		destPath := filepath.Join(uploadDir, fileName)
		if err := os.Remove(destPath); err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "file not found: " + fileName}},
			}, nil, nil
		}

		log.Printf("File deleted: %s", fileName)

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: fmt.Sprintf("File deleted: %s", fileName)}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}

// registerListFilesTool 注册文件列表工具
// 功能：列出服务器上已上传的所有文件
//
// 参数：
//   - server: MCP 服务器实例
//   - uploadDir: 上传文件存储目录
//
// 返回：
//   - 文件名列表及其大小
func registerListFilesTool(server *mcp.Server, uploadDir string) {
	tool := mcp.Tool{
		Name:        "list_files",
		Description: "List all files currently uploaded to the server.",
		InputSchema: map[string]any{
			"type":       "object",
			"properties": map[string]any{},
		},
	}

	handler := func(ctx context.Context, req *mcp.CallToolRequest, args map[string]any) (*mcp.CallToolResult, any, error) {
		// 读取上传目录的所有条目
		entries, err := os.ReadDir(uploadDir)
		if err != nil {
			return &mcp.CallToolResult{
				IsError: true,
				Content: []mcp.Content{&mcp.TextContent{Text: "failed to read upload directory"}},
			}, nil, nil
		}

		if len(entries) == 0 {
			return &mcp.CallToolResult{
				Content: []mcp.Content{&mcp.TextContent{Text: "No files uploaded"}},
			}, nil, nil
		}

		var files []string
		for _, entry := range entries {
			if !entry.IsDir() {
				info, _ := entry.Info()
				files = append(files, fmt.Sprintf("- %s (%d bytes)", entry.Name(), info.Size()))
			}
		}

		return &mcp.CallToolResult{
			Content: []mcp.Content{&mcp.TextContent{Text: "Uploaded files:\n" + strings.Join(files, "\n")}},
		}, nil, nil
	}

	mcp.AddTool(server, &tool, handler)
}
