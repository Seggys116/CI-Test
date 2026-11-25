package ciutil_test

import (
	"bytes"
	"errors"
	"testing"

	"ciutil/ciutil"
)

func TestBase64Vectors(t *testing.T) {
	tests := []struct {
		name string
		in   []byte
		want string
	}{
		{"empty", []byte(""), ""},
		{"f", []byte("f"), "Zg=="},
		{"fo", []byte("fo"), "Zm8="},
		{"foo", []byte("foo"), "Zm9v"},
		{"foob", []byte("foob"), "Zm9vYg=="},
		{"fooba", []byte("fooba"), "Zm9vYmE="},
		{"foobar", []byte("foobar"), "Zm9vYmFy"},
		{"symbols", []byte{0x00, 0xFF, 0xFE}, "AP/+"},
		{"plus slash", []byte{0xFB, 0xFF, 0xBF}, "+/+/"},
		{"all ones", []byte{0xFF, 0xFF, 0xFF}, "////"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := ciutil.Base64Encode(tt.in); got != tt.want {
				t.Errorf("Base64Encode = %q, want %q", got, tt.want)
			}
			got, err := ciutil.Base64Decode(tt.want)
			if err != nil {
				t.Fatalf("Base64Decode(%q): %v", tt.want, err)
			}
			if !bytes.Equal(got, tt.in) {
				t.Errorf("Base64Decode(%q) = %x, want %x", tt.want, got, tt.in)
			}
		})
	}
}

func TestBase64DecodeRejects(t *testing.T) {
	tests := []struct {
		name string
		in   string
		want error
	}{
		{"one char", "Z", ciutil.ErrBase64Length},
		{"two chars", "Zg", ciutil.ErrBase64Length},
		{"three chars", "Zg=", ciutil.ErrBase64Length},
		{"five chars", "Zm9vY", ciutil.ErrBase64Length},
		{"three pads", "Zg===", ciutil.ErrBase64Length},
		{"padding in the middle", "Zg==Zg==", ciutil.ErrBase64Padding},
		{"padding in the middle of three groups", "Zm8=Zm8=Zm8=", ciutil.ErrBase64Padding},
		{"only pads", "====", ciutil.ErrBase64Padding},
		{"three pads in a group", "Z===", ciutil.ErrBase64Padding},
		{"pad first", "=Zg=", ciutil.ErrBase64Padding},
		{"data after pad", "Zm=v", ciutil.ErrBase64Padding},
		{"data after double pad", "Zg=a", ciutil.ErrBase64Padding},
		{"trailing bits two pads", "Zh==", ciutil.ErrBase64TrailingBits},
		{"trailing bits one pad", "Zm9=", ciutil.ErrBase64TrailingBits},
		{"trailing bits later group", "Zm9vZh==", ciutil.ErrBase64TrailingBits},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := ciutil.Base64Decode(tt.in)
			if !errors.Is(err, tt.want) {
				t.Errorf("Base64Decode(%q) error = %v, want %v", tt.in, err, tt.want)
			}
			if got != nil {
				t.Errorf("Base64Decode(%q) returned %x with an error", tt.in, got)
			}
		})
	}
}

func TestBase64DecodeRejectsBytesOutsideTheAlphabet(t *testing.T) {
	tests := []struct {
		name  string
		in    string
		index int
		b     byte
	}{
		{"url-safe minus", "Zm9-", 3, '-'},
		{"url-safe underscore", "Zm9_", 3, '_'},
		{"url-safe in second group", "Zm9vZ_9v", 5, '_'},
		{"space", "Zm9 ", 3, ' '},
		{"leading space", " Zm9", 0, ' '},
		{"newline", "Zm9\n", 3, '\n'},
		{"tab", "Zm9\t", 3, '\t'},
		{"carriage return", "Zm9\r", 3, '\r'},
		{"star", "Zm9*", 3, '*'},
		{"non-ascii", "éé", 0, 0xC3},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			_, err := ciutil.Base64Decode(tt.in)
			var byteErr *ciutil.Base64ByteError
			if !errors.As(err, &byteErr) {
				t.Fatalf("Base64Decode(%q) error = %v, want *Base64ByteError", tt.in, err)
			}
			if byteErr.Index != tt.index || byteErr.Byte != tt.b {
				t.Errorf("got index %d byte 0x%02x, want index %d byte 0x%02x",
					byteErr.Index, byteErr.Byte, tt.index, tt.b)
			}
		})
	}
}

func TestBase64RoundTripEveryLengthUpTo300(t *testing.T) {
	data := pseudoRandom(300)
	for n := 0; n <= 300; n++ {
		encoded := ciutil.Base64Encode(data[:n])
		if want := (n + 2) / 3 * 4; len(encoded) != want {
			t.Fatalf("length %d: encoded length %d, want %d", n, len(encoded), want)
		}
		decoded, err := ciutil.Base64Decode(encoded)
		if err != nil {
			t.Fatalf("length %d: %v", n, err)
		}
		if !bytes.Equal(decoded, data[:n]) {
			t.Fatalf("length %d: round trip mismatch", n)
		}
	}
}

func TestBase64ErrorMessages(t *testing.T) {
	_, err := ciutil.Base64Decode("Zm9-")
	if got, want := err.Error(), "base64: invalid byte 0x2d at index 3"; got != want {
		t.Errorf("got %q, want %q", got, want)
	}
}

func FuzzBase64RoundTrip(f *testing.F) {
	f.Add([]byte(""))
	f.Add([]byte("f"))
	f.Add([]byte("foobar"))
	f.Add([]byte{0x00, 0xFF, 0xFE})
	f.Add([]byte("Zm9vYg=="))
	f.Add([]byte("Zh=="))
	f.Add(pseudoRandom(100))
	f.Fuzz(func(t *testing.T, data []byte) {
		encoded := ciutil.Base64Encode(data)
		decoded, err := ciutil.Base64Decode(encoded)
		if err != nil {
			t.Fatalf("Base64Decode(Base64Encode(%x)) failed: %v", data, err)
		}
		if !bytes.Equal(decoded, data) {
			t.Fatalf("round trip of %x gave %x", data, decoded)
		}

		// the decoder is strict, so anything it accepts re-encodes to itself
		text := string(data)
		accepted, err := ciutil.Base64Decode(text)
		if err != nil {
			return
		}
		if again := ciutil.Base64Encode(accepted); again != text {
			t.Fatalf("accepted %q but it re-encodes to %q", text, again)
		}
	})
}
