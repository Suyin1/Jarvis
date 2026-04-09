package main

import (
	"fmt"

	"github.com/spf13/pflag"

	"github.com/errata-ai/vale/v3/internal/core"
)

// Style represents an externally-hosted style.
type Style struct {
	// User-provided fields.
	Author      string `json:"author"`
	Description string `json:"description"`
	Deps        string `json:"deps"`
	Feed        string `json:"feed"`
	Homepage    string `json:"homepage"`
	Name        string `json:"name"`
	URL         string `json:"url"`

	// Generated fields.
	HasUpdate bool `json:"has_update"`
	InLibrary bool `json:"in_library"`
	Installed bool `json:"installed"`
	Addon     bool `json:"addon"`
}

// Meta represents an installed style's meta data.
type Meta struct {
	Author      string   `json:"author"`
	Coverage    float64  `json:"coverage"`
	Description string   `json:"description"`
	Email       string   `json:"email"`
	Feed        string   `json:"feed"`
	Lang        string   `json:"lang"`
	License     string   `json:"license"`
	Name        string   `json:"name"`
	Sources     []string `json:"sources"`
	URL         string   `json:"url"`
	Vale        string   `json:"vale_version"`
	Version     string   `json:"version"`
}

func init() {
	pflag.BoolVar(&Flags.Remote, "mode-rev-compat", false,
		"prioritize local Vale configurations")
	pflag.StringVar(&Flags.Built, "built", "", "post-processed file path")

	// [INTRANET-SAFE] install command is DISABLED
	// Actions["install"] = install
}

// [INTRANET-SAFE] fetch is DISABLED to prevent downloading from external URLs
func fetch(src, dst string) error {
	return fmt.Errorf(
		"[INTRANET-SAFE] External URL fetching is disabled. Cannot fetch '%s'. "+
			"Please download packages manually and place them in the styles directory.", src)
}

// [INTRANET-SAFE] install is DISABLED to prevent downloading from external URLs
func install(args []string, flags *core.CLIFlags) error {
	return fmt.Errorf(
		"[INTRANET-SAFE] install command is disabled. "+
			"Please download packages manually from trusted sources.")
}
