package ciutil_test

import (
	"errors"
	"math"
	"testing"

	"ciutil/ciutil"
)

func TestRomanVectors(t *testing.T) {
	tests := []struct {
		n    int
		want string
	}{
		{1, "I"},
		{4, "IV"},
		{9, "IX"},
		{14, "XIV"},
		{40, "XL"},
		{90, "XC"},
		{400, "CD"},
		{1994, "MCMXCIV"},
		{1999, "MCMXCIX"},
		{2024, "MMXXIV"},
		{3999, "MMMCMXCIX"},
	}
	for _, tt := range tests {
		got, err := ciutil.ToRoman(tt.n)
		if err != nil || got != tt.want {
			t.Errorf("ToRoman(%d) = %q, %v; want %q", tt.n, got, err, tt.want)
		}
		back, err := ciutil.FromRoman(tt.want)
		if err != nil || back != tt.n {
			t.Errorf("FromRoman(%q) = %d, %v; want %d", tt.want, back, err, tt.n)
		}
	}
}

func TestToRomanRejectsOutOfRange(t *testing.T) {
	for _, n := range []int{0, 4000, 4001, -1, -3999, math.MinInt32, math.MaxInt32} {
		got, err := ciutil.ToRoman(n)
		if !errors.Is(err, ciutil.ErrRomanRange) || got != "" {
			t.Errorf("ToRoman(%d) = %q, %v; want range error", n, got, err)
		}
	}
}

func TestFromRomanRejectsNonCanonicalForms(t *testing.T) {
	inputs := []string{
		"", "IIII", "VX", "IC", "XM", "VV", "IIX", "xiv", "Xiv",
		"IVI", "VIV", "IXI", "IIV", "XXXX", "CCCC", "MMMM", "LL", "DD",
		"XVX", "MCMC", "CMD", "CDC", "IL", "XD", "MMMMCMXCIX",
		" X", "X ", "X\n", "I I", "N", "0", "Ⅻ",
	}
	for _, s := range inputs {
		got, err := ciutil.FromRoman(s)
		if !errors.Is(err, ciutil.ErrRomanInvalid) || got != 0 {
			t.Errorf("FromRoman(%q) = %d, %v; want invalid error", s, got, err)
		}
	}
}

func TestRomanRoundTripWholeRange(t *testing.T) {
	seen := make(map[string]int, 3999)
	for n := 1; n <= 3999; n++ {
		numeral, err := ciutil.ToRoman(n)
		if err != nil {
			t.Fatalf("ToRoman(%d): %v", n, err)
		}
		back, err := ciutil.FromRoman(numeral)
		if err != nil || back != n {
			t.Fatalf("FromRoman(%q) = %d, %v; want %d", numeral, back, err, n)
		}
		if previous, dup := seen[numeral]; dup {
			t.Fatalf("%d and %d both map to %q", previous, n, numeral)
		}
		seen[numeral] = n
	}
}
