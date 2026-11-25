package ciutil_test

import (
	"errors"
	"reflect"
	"testing"

	"ciutil/ciutil"
)

var semverChain = []string{
	"1.0.0-alpha",
	"1.0.0-alpha.1",
	"1.0.0-alpha.beta",
	"1.0.0-beta",
	"1.0.0-beta.2",
	"1.0.0-beta.11",
	"1.0.0-rc.1",
	"1.0.0",
	"2.0.0",
	"2.1.0",
	"2.1.1",
}

func mustParse(t testing.TB, s string) ciutil.Version {
	t.Helper()
	v, err := ciutil.ParseVersion(s)
	if err != nil {
		t.Fatalf("ParseVersion(%q): %v", s, err)
	}
	return v
}

func TestParseVersionComponents(t *testing.T) {
	v := mustParse(t, "1.2.3-rc.1+build.5")
	if v.Major != 1 || v.Minor != 2 || v.Patch != 3 {
		t.Errorf("core = %d.%d.%d, want 1.2.3", v.Major, v.Minor, v.Patch)
	}
	if want := []string{"rc", "1"}; !reflect.DeepEqual(v.Prerelease, want) {
		t.Errorf("Prerelease = %q, want %q", v.Prerelease, want)
	}
	if want := []string{"build", "5"}; !reflect.DeepEqual(v.Build, want) {
		t.Errorf("Build = %q, want %q", v.Build, want)
	}

	plain := mustParse(t, "10.20.30")
	if len(plain.Prerelease) != 0 || len(plain.Build) != 0 {
		t.Errorf("plain version has identifiers: %q %q", plain.Prerelease, plain.Build)
	}
}

func TestParseVersionAccepts(t *testing.T) {
	inputs := []string{
		"0.0.0",
		"1.0.0-0",
		"1.0.0-0a",
		"1.0.0-x-y-z.-",
		"1.0.0+001",
		"1.0.0-rc.1+build.5",
		"1.0.0-alpha.beta",
		"1.0.0-alpha+001",
		"1.0.0+20130313144700",
		"1.0.0-0.3.7",
		"1.0.0-a-b.c-d+e-f.g",
		"1.0.0---",
		"999.999.999",
		"18446744073709551615.18446744073709551615.18446744073709551615",
	}
	for _, s := range inputs {
		v := mustParse(t, s)
		if got := v.String(); got != s {
			t.Errorf("String() of %q = %q", s, got)
		}
		if again := mustParse(t, v.String()); !reflect.DeepEqual(again, v) {
			t.Errorf("re-parse of %q differs: %+v vs %+v", s, again, v)
		}
	}
}

func TestParseVersionRejects(t *testing.T) {
	tests := []struct {
		in   string
		want error
	}{
		{"", ciutil.ErrSemverCore},
		{"1", ciutil.ErrSemverCore},
		{"1.0", ciutil.ErrSemverCore},
		{"1.0.0.0", ciutil.ErrSemverCore},
		{"v1.0.0", ciutil.ErrSemverCore},
		{"01.0.0", ciutil.ErrSemverLeadingZero},
		{"1.00.0", ciutil.ErrSemverLeadingZero},
		{"1.0.00", ciutil.ErrSemverLeadingZero},
		{"1.0.0-", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0+", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0-01", ciutil.ErrSemverLeadingZero},
		{"1.0.0-alpha.01", ciutil.ErrSemverLeadingZero},
		{"1.0.0-a..b", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0-alpha.", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0-.alpha", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0+a..b", ciutil.ErrSemverEmptyIdentifier},
		{"1.0.0-a_b", ciutil.ErrSemverInvalidIdentifier},
		{"1.0.0+a_b", ciutil.ErrSemverInvalidIdentifier},
		{"1.0.0+a+b", ciutil.ErrSemverInvalidIdentifier},
		{"1.0.0-é", ciutil.ErrSemverInvalidIdentifier},
		{"-1.0.0", ciutil.ErrSemverCore},
		{"+1.0.0", ciutil.ErrSemverCore},
		{" 1.0.0", ciutil.ErrSemverCore},
		{"1.0.0 ", ciutil.ErrSemverCore},
		{"1.0.0\n", ciutil.ErrSemverCore},
		{"1.0.0-alpha+", ciutil.ErrSemverEmptyIdentifier},
		{"1..0", ciutil.ErrSemverCore},
		{"..", ciutil.ErrSemverCore},
		{"1.a.0", ciutil.ErrSemverCore},
		{"1.0.-1", ciutil.ErrSemverCore},
		{"1.0.+1", ciutil.ErrSemverCore},
		{"18446744073709551616.0.0", ciutil.ErrSemverOverflow},
		{"0.18446744073709551616.0", ciutil.ErrSemverOverflow},
		{"0.0.99999999999999999999", ciutil.ErrSemverOverflow},
	}
	for _, tt := range tests {
		if _, err := ciutil.ParseVersion(tt.in); !errors.Is(err, tt.want) {
			t.Errorf("ParseVersion(%q) error = %v, want %v", tt.in, err, tt.want)
		}
	}
}

func TestVersionOrderingChainForAllOrderedPairs(t *testing.T) {
	chain := make([]ciutil.Version, len(semverChain))
	for i, s := range semverChain {
		chain[i] = mustParse(t, s)
	}
	for i := range chain {
		for j := range chain {
			want := 0
			if i < j {
				want = -1
			} else if i > j {
				want = 1
			}
			if got := chain[i].Compare(chain[j]); got != want {
				t.Errorf("Compare(%s, %s) = %d, want %d", semverChain[i], semverChain[j], got, want)
			}
		}
	}
}

func TestVersionCompareIdentifierRules(t *testing.T) {
	tests := []struct {
		a, b string
		want int
	}{
		{"1.0.0-beta.2", "1.0.0-beta.11", -1},
		{"1.0.0-1", "1.0.0-a", -1},
		{"1.0.0-B", "1.0.0-a", -1},
		{"1.0.0-a-b", "1.0.0-a", 1},
		{"1.0.0-alpha.1", "1.0.0-alpha", 1},
		{"1.0.0-0a", "1.0.0-0", 1},
		{"1.0.0-9", "1.0.0-10", -1},
		{"1.0.0-99999999999999999999", "1.0.0-100000000000000000000", -1},
		{"0.9.9", "1.0.0", -1},
		{"1.0.0", "1.0.0", 0},
		{"2.1.1", "2.1.0", 1},
		{"1.10.0", "1.9.0", 1},
		{"1.0.10", "1.0.9", 1},
		{"10.0.0", "9.0.0", 1},
	}
	for _, tt := range tests {
		a, b := mustParse(t, tt.a), mustParse(t, tt.b)
		if got := a.Compare(b); got != tt.want {
			t.Errorf("Compare(%s, %s) = %d, want %d", tt.a, tt.b, got, tt.want)
		}
		if got := b.Compare(a); got != -tt.want {
			t.Errorf("Compare(%s, %s) = %d, want %d", tt.b, tt.a, got, -tt.want)
		}
	}
}

func TestVersionCompareIgnoresBuildMetadata(t *testing.T) {
	tests := []struct {
		a, b string
		want int
	}{
		{"1.0.0+a", "1.0.0+b", 0},
		{"1.0.0-rc.1+a", "1.0.0-rc.1", 0},
		{"1.0.0+z", "1.0.1+a", -1},
	}
	for _, tt := range tests {
		if got := mustParse(t, tt.a).Compare(mustParse(t, tt.b)); got != tt.want {
			t.Errorf("Compare(%s, %s) = %d, want %d", tt.a, tt.b, got, tt.want)
		}
	}
}

func TestVersionCompareIsAntisymmetric(t *testing.T) {
	samples := append([]string{"0.0.0", "1.0.0+x", "1.2.3-rc.1", "1.0.0-1"}, semverChain...)
	for _, a := range samples {
		for _, b := range samples {
			forward := mustParse(t, a).Compare(mustParse(t, b))
			backward := mustParse(t, b).Compare(mustParse(t, a))
			if forward != -backward {
				t.Errorf("Compare(%s, %s) = %d but reverse = %d", a, b, forward, backward)
			}
		}
	}
}

func FuzzSemverParse(f *testing.F) {
	seeds := []string{
		"", "1", "1.0", "0.0.0", "1.0.0", "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-0",
		"1.0.0-0a", "1.0.0-01", "1.0.0+001", "1.0.0-rc.1+build.5", "1.0.0-x-y-z.-",
		"1.0.0-", "1.0.0+", "01.0.0", "v1.0.0", " 1.0.0", "1.0.0-a..b", "1.0.0-a_b",
		"18446744073709551615.0.0", "18446744073709551616.0.0",
	}
	for _, s := range seeds {
		f.Add(s)
	}
	f.Fuzz(func(t *testing.T, s string) {
		v, err := ciutil.ParseVersion(s)
		if err != nil {
			return
		}
		if got := v.String(); got != s {
			t.Fatalf("String() of parsed %q = %q", s, got)
		}
		again, err := ciutil.ParseVersion(v.String())
		if err != nil {
			t.Fatalf("re-parse of %q failed: %v", s, err)
		}
		if !reflect.DeepEqual(again, v) {
			t.Fatalf("re-parse of %q differs: %+v vs %+v", s, again, v)
		}
		if v.Compare(again) != 0 {
			t.Fatalf("%q does not compare equal to itself", s)
		}
	})
}
