//go:build !http
// +build !http

package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"

	"github.com/modelcontextprotocol/go-sdk/mcp"
	"vale-mcp-server/internal/handlers"
)

func main() {
	allowedDir := os.Getenv("VALE_ALLOWED_DIR")
	if allowedDir == "" {
		allowedDir = "."
		log.Printf("Warning: VALE_ALLOWED_DIR not set, using current directory: %s", allowedDir)
	}

	server := mcp.NewServer(&mcp.Implementation{Name: "vale-mcp-server", Version: "1.0.0"}, nil)

	handlers.RegisterAllTools(server, allowedDir)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		sig := <-sigChan
		log.Printf("Received signal: %v, shutting down...", sig)
		cancel()
	}()

	if err := server.Run(ctx, &mcp.StdioTransport{}); err != nil {
		log.Fatalf("Server error: %v", err)
	}

	log.Println("Server stopped")
}
