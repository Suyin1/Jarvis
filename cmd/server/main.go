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
	}

	server := mcp.NewServer("vale-mcp-server", "1.0.0")
	handlers.RegisterAllTools(server, allowedDir)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigChan
		log.Println("Shutting down...")
		cancel()
	}()

	if err := server.Run(ctx); err != nil {
		log.Fatalf("Server error: %v", err)
	}
}
