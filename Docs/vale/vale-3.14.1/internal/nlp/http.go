package nlp

import (
	"errors"

	"github.com/jdkato/twine/nlp/tag"
)

// [INTRANET-SAFE] External NLP API calls are DISABLED

type SegmentResult struct {
	Sents []string
}

type TagResult struct {
	Tokens []tag.Token
}

func post(url string) ([]byte, error) {
	return []byte{}, errors.New(
		"[INTRANET-SAFE] External HTTP POST requests are disabled")
}

func doSegment(text, lang, apiURL string) (SegmentResult, error) {
	return SegmentResult{}, errors.New(
		"[INTRANET-SAFE] External NLP segment API is disabled")
}

func pos(text, lang, apiURL string) (TagResult, error) {
	return TagResult{}, errors.New(
		"[INTRANET-SAFE] External NLP POS API is disabled")
}
