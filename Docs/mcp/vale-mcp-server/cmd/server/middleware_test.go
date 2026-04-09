package main

import (
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestAPIKeyAuth_ValidKey(t *testing.T) {
	auth := NewAPIKeyAuth([]string{"valid-key-123", "another-key"})

	req := httptest.NewRequest("GET", "/test", nil)
	req.Header.Set("X-API-Key", "valid-key-123")

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}
}

func TestAPIKeyAuth_InvalidKey(t *testing.T) {
	auth := NewAPIKeyAuth([]string{"valid-key-123"})

	req := httptest.NewRequest("GET", "/test", nil)
	req.Header.Set("X-API-Key", "invalid-key")

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusUnauthorized {
		t.Errorf("Expected status 401, got %d", w.Code)
	}
}

func TestAPIKeyAuth_MissingKey(t *testing.T) {
	auth := NewAPIKeyAuth([]string{"valid-key-123"})

	req := httptest.NewRequest("GET", "/test", nil)

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusUnauthorized {
		t.Errorf("Expected status 401, got %d", w.Code)
	}
}

func TestAPIKeyAuth_NoAuthRequired(t *testing.T) {
	auth := NewAPIKeyAuth([]string{})

	req := httptest.NewRequest("GET", "/test", nil)

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}
}

func TestAPIKeyAuth_KeyFromQuery(t *testing.T) {
	auth := NewAPIKeyAuth([]string{"query-key-456"})

	req := httptest.NewRequest("GET", "/test?api_key=query-key-456", nil)

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))

	handler.ServeHTTP(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}
}

func TestAPIKeyAuth_RateLimitExceeded(t *testing.T) {
	auth := NewAPIKeyAuth([]string{"test-key"})
	auth.maxRequests = 3
	auth.windowSeconds = 60

	req := httptest.NewRequest("GET", "/test", nil)

	for i := 0; i < 3; i++ {
		w := httptest.NewRecorder()
		handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusOK)
		}))
		handler.ServeHTTP(w, req)
	}

	w := httptest.NewRecorder()
	handler := auth.Middleware(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))
	handler.ServeHTTP(w, req)

	if w.Code != http.StatusTooManyRequests {
		t.Errorf("Expected status 429 (rate limit), got %d", w.Code)
	}
}

func TestSanitizeLogData_Truncation(t *testing.T) {
	longData := string(make([]byte, 1000))
	for i := range longData {
		longData = longData[:i] + "a"
	}

	result := SanitizeLogData(longData)

	if len(result) > 600 {
		t.Errorf("Expected truncated data, got length %d", len(result))
	}
}

func TestSanitizeLogData_ShortData(t *testing.T) {
	shortData := "short log message"

	result := SanitizeLogData(shortData)

	if result != shortData {
		t.Errorf("Expected unchanged data, got %s", result)
	}
}
