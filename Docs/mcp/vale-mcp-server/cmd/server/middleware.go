package main

import (
	"net/http"
	"sync"
	"time"
)

type APIKeyAuth struct {
	validKeys     []string
	mu            sync.Mutex
	requests      map[string][]time.Time
	maxRequests   int
	windowSeconds int
}

func NewAPIKeyAuth(keys []string) *APIKeyAuth {
	return &APIKeyAuth{
		validKeys:     keys,
		requests:      make(map[string][]time.Time),
		maxRequests:   60,
		windowSeconds: 60,
	}
}

func (a *APIKeyAuth) Middleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		clientIP := r.RemoteAddr
		if !a.checkRateLimit(clientIP) {
			http.Error(w, "rate limit exceeded", http.StatusTooManyRequests)
			return
		}

		apiKey := r.Header.Get("X-API-Key")
		if apiKey == "" {
			apiKey = r.URL.Query().Get("api_key")
		}

		if len(a.validKeys) > 0 && apiKey == "" {
			http.Error(w, "missing API key", http.StatusUnauthorized)
			return
		}

		if len(a.validKeys) > 0 && !a.containsKey(apiKey) {
			http.Error(w, "invalid API key", http.StatusUnauthorized)
			return
		}

		next.ServeHTTP(w, r)
	})
}

func (a *APIKeyAuth) checkRateLimit(key string) bool {
	a.mu.Lock()
	defer a.mu.Unlock()

	now := time.Now()
	window := now.Add(-time.Duration(a.windowSeconds) * time.Second)

	var valid []time.Time
	for _, t := range a.requests[key] {
		if t.After(window) {
			valid = append(valid, t)
		}
	}

	if len(valid) >= a.maxRequests {
		a.requests[key] = valid
		return false
	}

	a.requests[key] = append(valid, now)
	return true
}

func (a *APIKeyAuth) containsKey(key string) bool {
	for _, valid := range a.validKeys {
		if valid == key {
			return true
		}
	}
	return false
}

func SanitizeLogData(data string) string {
	if len(data) > 500 {
		data = data[:500] + "...[truncated]"
	}
	return data
}

type SanitizeResponseWriter struct {
	http.ResponseWriter
	statusCode int
}

func (w *SanitizeResponseWriter) WriteHeader(code int) {
	w.statusCode = code
	w.ResponseWriter.WriteHeader(code)
}

func LoggingMiddleware(next http.Handler, logger *Logger) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		sw := &SanitizeResponseWriter{ResponseWriter: w, statusCode: 200}

		next.ServeHTTP(sw, r)

		duration := time.Since(start)
		logger.Printf("method=%s path=%s status=%d duration=%v",
			r.Method,
			r.URL.Path,
			sw.statusCode,
			duration,
		)
	})
}

type Logger struct{}

func (l *Logger) Printf(format string, v ...interface{}) {}
