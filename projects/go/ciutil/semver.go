package ciutil

import (
	"cmp"
	"errors"
	"strconv"
	"strings"
)

var ErrSemverCore = errors.New("semver: expected MAJOR.MINOR.PATCH")

var ErrSemverLeadingZero = errors.New("semver: numeric field has a leading zero")

var ErrSemverOverflow = errors.New("semver: numeric field does not fit in 64 bits")

var ErrSemverEmptyIdentifier = errors.New("semver: empty identifier")

var ErrSemverInvalidIdentifier = errors.New("semver: identifier contains an invalid character")

// Version is a parsed semantic version. Compare ignores Build.
type Version struct {
	Major      uint64
	Minor      uint64
	Patch      uint64
	Prerelease []string
	Build      []string
}

func isNumeric(s string) bool {
	if s == "" {
		return false
	}
	for i := 0; i < len(s); i++ {
		if s[i] < '0' || s[i] > '9' {
			return false
		}
	}
	return true
}

func hasLeadingZero(s string) bool {
	return len(s) > 1 && s[0] == '0'
}

func isIdentifierByte(c byte) bool {
	switch {
	case c >= '0' && c <= '9', c >= 'a' && c <= 'z', c >= 'A' && c <= 'Z', c == '-':
		return true
	}
	return false
}

func parseNumber(s string) (uint64, error) {
	if !isNumeric(s) {
		return 0, ErrSemverCore
	}
	if hasLeadingZero(s) {
		return 0, ErrSemverLeadingZero
	}
	n, err := strconv.ParseUint(s, 10, 64)
	if err != nil {
		return 0, ErrSemverOverflow
	}
	return n, nil
}

func parseIdentifiers(s string, prerelease bool) ([]string, error) {
	identifiers := strings.Split(s, ".")
	for _, id := range identifiers {
		if id == "" {
			return nil, ErrSemverEmptyIdentifier
		}
		for i := 0; i < len(id); i++ {
			if !isIdentifierByte(id[i]) {
				return nil, ErrSemverInvalidIdentifier
			}
		}
		if prerelease && isNumeric(id) && hasLeadingZero(id) {
			return nil, ErrSemverLeadingZero
		}
	}
	return identifiers, nil
}

// ParseVersion parses MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD].
func ParseVersion(s string) (Version, error) {
	var v Version
	rest := s
	if i := strings.IndexByte(rest, '+'); i >= 0 {
		build, err := parseIdentifiers(rest[i+1:], false)
		if err != nil {
			return Version{}, err
		}
		v.Build = build
		rest = rest[:i]
	}
	if i := strings.IndexByte(rest, '-'); i >= 0 {
		prerelease, err := parseIdentifiers(rest[i+1:], true)
		if err != nil {
			return Version{}, err
		}
		v.Prerelease = prerelease
		rest = rest[:i]
	}
	fields := strings.Split(rest, ".")
	if len(fields) != 3 {
		return Version{}, ErrSemverCore
	}
	var numbers [3]uint64
	for i, field := range fields {
		n, err := parseNumber(field)
		if err != nil {
			return Version{}, err
		}
		numbers[i] = n
	}
	v.Major, v.Minor, v.Patch = numbers[0], numbers[1], numbers[2]
	return v, nil
}

// String formats the version so that ParseVersion accepts it again.
func (v Version) String() string {
	var sb strings.Builder
	sb.WriteString(strconv.FormatUint(v.Major, 10))
	sb.WriteByte('.')
	sb.WriteString(strconv.FormatUint(v.Minor, 10))
	sb.WriteByte('.')
	sb.WriteString(strconv.FormatUint(v.Patch, 10))
	if len(v.Prerelease) > 0 {
		sb.WriteByte('-')
		sb.WriteString(strings.Join(v.Prerelease, "."))
	}
	if len(v.Build) > 0 {
		sb.WriteByte('+')
		sb.WriteString(strings.Join(v.Build, "."))
	}
	return sb.String()
}

func compareIdentifiers(a, b string) int {
	aNumeric, bNumeric := isNumeric(a), isNumeric(b)
	switch {
	case aNumeric && bNumeric:
		// no leading zeros, so a longer digit string is a larger number
		return cmp.Or(cmp.Compare(len(a), len(b)), cmp.Compare(a, b))
	case aNumeric:
		return -1
	case bNumeric:
		return 1
	default:
		return cmp.Compare(a, b)
	}
}

func comparePrerelease(a, b []string) int {
	switch {
	case len(a) == 0 && len(b) == 0:
		return 0
	case len(a) == 0:
		return 1
	case len(b) == 0:
		return -1
	}
	for i := 0; i < len(a) && i < len(b); i++ {
		if c := compareIdentifiers(a[i], b[i]); c != 0 {
			return c
		}
	}
	return cmp.Compare(len(a), len(b))
}

// Compare returns -1, 0 or 1 by SemVer precedence; build metadata is ignored.
func (v Version) Compare(other Version) int {
	return cmp.Or(
		cmp.Compare(v.Major, other.Major),
		cmp.Compare(v.Minor, other.Minor),
		cmp.Compare(v.Patch, other.Patch),
		comparePrerelease(v.Prerelease, other.Prerelease),
	)
}
