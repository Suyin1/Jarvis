//go:build http
// +build http

package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/modelcontextprotocol/go-sdk/mcp"
	"vale-mcp-server/internal/handlers"
)

func main() {
	allowedDir := os.Getenv("VALE_ALLOWED_DIR")
	if allowedDir == "" {
		allowedDir = "."
		log.Printf("Warning: VALE_ALLOWED_DIR not set, using current directory: %s", allowedDir)
	}

	port := os.Getenv("MCP_SERVER_PORT")
	if port == "" {
		port = "8080"
	}

	apiKey := os.Getenv("MCP_API_KEY")
	var validKeys []string
	if apiKey != "" {
		validKeys = []string{apiKey}
		log.Println("API Key authentication enabled")
	} else {
		log.Println("Warning: MCP_API_KEY not set, authentication disabled")
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

	httpHandler := mcp.NewStreamableHTTPHandler(
		func(req *http.Request) *mcp.Server {
			return server
		},
		&mcp.StreamableHTTPOptions{
			SessionTimeout: 60 * time.Second,
		},
	)

	auth := NewAPIKeyAuth(validKeys)
	handler := auth.Middleware(httpHandler)

	httpServer := &http.Server{
		Addr:    "127.0.0.1:" + port,
		Handler: handler,
	}

	go func() {
		log.Printf("MCP HTTP Server starting on 127.0.0.1:%s", port)
		log.Printf("Endpoint: http://localhost:%s/mcp", port)
		log.Printf("Security: listening on localhost only, API Key %s", map[bool]string{true: "enabled", false: "disabled"}[apiKey != ""])
		if err := httpServer.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("Server error: %v", err)
		}
	}()

	<-ctx.Done()
	log.Println("Shutting down server...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer shutdownCancel()

	if err := httpServer.Shutdown(shutdownCtx); err != nil {
		log.Printf("Server shutdown error: %v", err)
	}

	log.Println("Server stopped")
}
