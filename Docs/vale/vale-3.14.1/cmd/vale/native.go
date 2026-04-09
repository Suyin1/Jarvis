package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"

	"github.com/adrg/xdg"

	"github.com/errata-ai/vale/v3/internal/core"
	"github.com/errata-ai/vale/v3/internal/system"
)

const nativeHostName = "sh.vale.native"
const releaseURL = "https://github.com/errata-ai/vale-native/releases/download/%s/vale-native_%s.%s"

var supportedBrowsers = []string{
	"chrome",
	"firefox",
	"opera",
	"chromium",
	"edge",
}

var extensionByBrowser = map[string]string{
	"chrome": "chrome-extension://kfmjcegeklidlnjoechfggipjjjahedj/",
}

var (
	errMissingBrowser = errors.New("missing argument 'browser'")
	errInvalidBrowser = fmt.Errorf("invalid browser; must one of %v", supportedBrowsers)
	errMissingExt     = errors.New("no extension for the given browser")
)

type manifest struct {
	Name              string   `json:"name"`
	Description       string   `json:"description"`
	Path              string   `json:"path"`
	Type              string   `json:"type"`
	AllowedExtensions []string `json:"allowed_extensions,omitempty"`
	AllowedOrigins    []string `json:"allowed_origins,omitempty"`
}

// getNativeConfig returns the path to the native host's config file.
//
// NOTE: When the browser (e.g., Chrome) launches the native host, it does
// not have access to the user's shell environment. This is actually why we
// need a config file at all -- to tell the host where to find the Vale
// binary.
//
// The problem is, however, that we can't rely on `XDG_CONFIG_HOME` to be set,
// so we need to use the default value.
func getNativeConfig() (string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	name := system.Name()

	switch name {
	case "windows":
		cfg, notFound := xdg.ConfigFile("vale/native/config.json")
		if notFound != nil {
			return "", notFound
		}
		return cfg, nil
	case "linux":
		path := filepath.Join(home, ".config/vale/native/config.json")
		if err = system.Mkdir(filepath.Dir(path)); err != nil {
			return "", err
		}
		return path, nil
	case "darwin":
		path := filepath.Join(home, "Library/Application Support/vale/native/config.json")
		if err = system.Mkdir(filepath.Dir(path)); err != nil {
			return "", err
		}
		return path, nil
	default:
		return "", fmt.Errorf("unsupported OS: %s", name)
	}
}

func getExecName(name string) string {
	if system.IsWindows() {
		return name + ".exe"
	}
	return name
}

func getManifestDirs() (map[string]string, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, err
	}

	manifests := map[string]string{}
	switch system.Name() {
	case "linux":
		manifests = map[string]string{
			"chrome":   filepath.Join(home, ".config/google-chrome/NativeMessagingHosts"),
			"firefox":  filepath.Join(home, ".mozilla/native-messaging-hosts"),
			"opera":    filepath.Join(home, ".config/google-chrome/NativeMessagingHosts"),
			"chromium": filepath.Join(home, ".config/chromium/NativeMessagingHosts"),
		}
	case "darwin":
		manifests = map[string]string{
			"chrome":   filepath.Join(home, "Library/Application Support/Google/Chrome/NativeMessagingHosts"),
			"firefox":  filepath.Join(home, "Library/Application Support/Mozilla/NativeMessagingHosts"),
			"opera":    filepath.Join(home, "Library/Application Support/Google/Chrome/NativeMessagingHosts"),
			"chromium": filepath.Join(home, "Library/Application Support/Chromium/NativeMessagingHosts"),
			"edge":     filepath.Join(home, "Library/Application Support/Microsoft Edge/NativeMessagingHosts"),
		}
	}

	return manifests, nil
}

func getLocation(browser string) (map[string]string, error) {
	cfg, err := getNativeConfig()
	if err != nil {
		return nil, err
	}

	bin := filepath.Dir(cfg)
	if system.IsWindows() {
		return map[string]string{
			"appDir":      bin,
			"manifestDir": "",
		}, nil
	}

	manifestDirs, err := getManifestDirs()
	if err != nil {
		return nil, err
	}

	manifest := ""
	if found, ok := manifestDirs[browser]; ok {
		manifest = found
	}

	return map[string]string{
		"appDir":      bin,
		"manifestDir": manifest,
	}, nil
}

func writeNativeConfig() (string, error) {
	cfgFile, err := getNativeConfig()
	if err != nil {
		return "", err
	}

	exe, err := exec.LookPath("vale")
	if err != nil {
		return "", err
	}

	cfg := map[string]string{
		"path": exe,
	}

	jsonCfg, err := json.Marshal(cfg)
	if err != nil {
		return "", err
	}

	return cfgFile, os.WriteFile(cfgFile, jsonCfg, 0600)
}

func installNativeHostUnix(manifestData []byte, manifestFile string) error {
	err := os.WriteFile(manifestFile, manifestData, 0600)
	if err != nil {
		return err
	}
	return nil
}

func installNativeHostWindows(manifestData []byte, manifestFile, browser string) error {
	cfg, err := getNativeConfig()
	if err != nil {
		return err
	}

	manifestDir := filepath.Join(filepath.Dir(cfg), "manifest", browser)

	err = os.MkdirAll(manifestDir, os.ModePerm)
	if err != nil {
		return err
	}
	subdir := filepath.Join(manifestDir, manifestFile)

	err = os.WriteFile(subdir, manifestData, 0600)
	if err != nil {
		return err
	}

	err = setManifestRegistry(browser, subdir)
	if err != nil {
		return err
	}

	return nil
}

// [INTRANET-SAFE] getLatestHostRelease is DISABLED
func getLatestHostRelease() (string, error) {
	return "", errors.New(
		"[INTRANET-SAFE] Checking GitHub releases is disabled in intranet mode")
}

// [INTRANET-SAFE] hostDownloadURL is DISABLED
func hostDownloadURL() (string, error) {
	return "", errors.New(
		"[INTRANET-SAFE] Downloading native host binaries is disabled in intranet mode")
}

func installHost(manifestJSON []byte, manifestFile, browser string) error {
	name := system.Name()

	switch name {
	case "linux", "darwin":
		return installNativeHostUnix(manifestJSON, manifestFile)
	case "windows":
		return installNativeHostWindows(manifestJSON, manifestFile, browser)
	default:
		return fmt.Errorf("unsupported OS: %s", name)
	}
}

// [INTRANET-SAFE] installNativeHost is DISABLED
func installNativeHost(args []string, _ *core.CLIFlags) error {
	return core.NewE100("host-install", errors.New(
		"[INTRANET-SAFE] host-install is disabled. "+
			"Native messaging host installation requires network access."))
}

// [INTRANET-SAFE] uninstallNativeHost is DISABLED
func uninstallNativeHost(args []string, _ *core.CLIFlags) error {
	return core.NewE100("host-uninstall", errors.New(
		"[INTRANET-SAFE] host-uninstall is disabled. "+
			"Please manually remove native messaging host files if needed."))
}
