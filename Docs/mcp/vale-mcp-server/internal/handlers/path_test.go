package handlers

import (
	"os"
	"path/filepath"
	"testing"
)

func TestValidatePath_ValidPath(t *testing.T) {
	allowedDir := t.TempDir()
	testFile := filepath.Join(allowedDir, "test.txt")
	os.WriteFile(testFile, []byte("test"), 0644)

	result, err := validatePath(testFile, allowedDir)

	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}
	if result == "" {
		t.Errorf("Expected valid path, got empty string")
	}
}

func TestValidatePath_RelativePath(t *testing.T) {
	allowedDir := t.TempDir()
	subdir := filepath.Join(allowedDir, "subdir")
	os.MkdirAll(subdir, 0755)
	testFile := filepath.Join(subdir, "test.txt")
	os.WriteFile(testFile, []byte("test"), 0644)

	result, err := validatePath(testFile, allowedDir)

	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}
	if result == "" {
		t.Errorf("Expected valid path, got empty string")
	}
}

func TestValidatePath_PathTraversal_ParentDirectory(t *testing.T) {
	allowedDir := t.TempDir()
	testFile := filepath.Join(allowedDir, "..", "outside.txt")

	_, err := validatePath(testFile, allowedDir)

	if err == nil {
		t.Error("Expected error for path traversal, got nil")
	}
}

func TestValidatePath_PathTraversal_Encoded(t *testing.T) {
	allowedDir := t.TempDir()
	testFile := filepath.Join(allowedDir, "..", "..", "etc", "passwd")

	_, err := validatePath(testFile, allowedDir)

	if err == nil {
		t.Error("Expected error for encoded path traversal, got nil")
	}
}

func TestValidatePath_NonExistentPath(t *testing.T) {
	allowedDir := t.TempDir()
	testFile := filepath.Join(allowedDir, "nonexistent.txt")

	result, err := validatePath(testFile, allowedDir)

	if err != nil {
		t.Errorf("Expected no error for non-existent path, got %v", err)
	}
	if result == "" {
		t.Errorf("Expected path, got empty string")
	}
}

func TestValidatePath_SymlinkOutside(t *testing.T) {
	if os.PathSeparator != '\\' {
		allowedDir := t.TempDir()
		outsideDir := t.TempDir()

		outsideFile := filepath.Join(outsideDir, "secret.txt")
		os.WriteFile(outsideFile, []byte("secret"), 0644)

		symlink := filepath.Join(allowedDir, "link.txt")
		os.Symlink(outsideFile, symlink)

		_, err := validatePath(symlink, allowedDir)

		if err == nil {
			t.Error("Expected error for symlink pointing outside, got nil")
		}
	} else {
		t.Skip("Skipping symlink test on Windows (requires admin or developer mode)")
	}
}

func TestValidatePath_SymlinkInside(t *testing.T) {
	allowedDir := t.TempDir()

	insideDir := filepath.Join(allowedDir, "subdir")
	os.MkdirAll(insideDir, 0755)

	targetFile := filepath.Join(insideDir, "file.txt")
	os.WriteFile(targetFile, []byte("test"), 0644)

	symlink := filepath.Join(allowedDir, "link.txt")
	os.Symlink(targetFile, symlink)

	result, err := validatePath(symlink, allowedDir)

	if err != nil {
		t.Errorf("Expected no error for symlink inside, got %v", err)
	}
	if result == "" {
		t.Errorf("Expected resolved path, got empty string")
	}
}

func TestValidatePath_EmptyPath(t *testing.T) {
	allowedDir := t.TempDir()

	_, err := validatePath("", allowedDir)

	if err == nil {
		t.Error("Expected error for empty path, got nil")
	}
}

func TestValidatePath_ReservedCharacters(t *testing.T) {
	allowedDir := t.TempDir()

	testCases := []string{
		"..\\..\\windows\\system32",
		"/etc/passwd",
		"\\\\UNC\\path\\to\\share",
		"file\x00.txt",
		"file\x1a.txt",
	}

	for _, tc := range testCases {
		_, err := validatePath(tc, allowedDir)
		if err != nil {
			t.Logf("Correctly rejected: %s - %v", tc, err)
		}
	}
}

func TestValidatePath_WindowsDriveLetter(t *testing.T) {
	allowedDir := t.TempDir()

	_, err := validatePath("C:\\Windows\\System32", allowedDir)

	if err == nil {
		t.Error("Expected error for Windows system path, got nil")
	}
}

func TestValidatePath_SameAsAllowedDir(t *testing.T) {
	allowedDir := t.TempDir()

	result, err := validatePath(allowedDir, allowedDir)

	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}
	if result != allowedDir {
		t.Errorf("Expected same path, got %s", result)
	}
}
