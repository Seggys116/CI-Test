package ciutil

import (
	"errors"
	"strings"
)

const romanMax = 3999

var ErrRomanRange = errors.New("roman: number must be between 1 and 3999")

var ErrRomanInvalid = errors.New("roman: not a canonical roman numeral")

var romanNumerals = []struct {
	value  int
	symbol string
}{
	{1000, "M"},
	{900, "CM"},
	{500, "D"},
	{400, "CD"},
	{100, "C"},
	{90, "XC"},
	{50, "L"},
	{40, "XL"},
	{10, "X"},
	{9, "IX"},
	{5, "V"},
	{4, "IV"},
	{1, "I"},
}

// ToRoman converts 1 through 3999 to its canonical uppercase numeral.
func ToRoman(n int) (string, error) {
	if n < 1 || n > romanMax {
		return "", ErrRomanRange
	}
	var sb strings.Builder
	for _, numeral := range romanNumerals {
		for n >= numeral.value {
			sb.WriteString(numeral.symbol)
			n -= numeral.value
		}
	}
	return sb.String(), nil
}

// FromRoman parses a numeral; anything ToRoman would not produce is rejected.
func FromRoman(s string) (int, error) {
	rest := s
	total := 0
	for _, numeral := range romanNumerals {
		for strings.HasPrefix(rest, numeral.symbol) {
			total += numeral.value
			if total > romanMax {
				return 0, ErrRomanInvalid
			}
			rest = rest[len(numeral.symbol):]
		}
	}
	canonical, err := ToRoman(total)
	if err != nil || rest != "" || canonical != s {
		return 0, ErrRomanInvalid
	}
	return total, nil
}
