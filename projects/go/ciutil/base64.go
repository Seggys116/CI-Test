package ciutil

import (
	"errors"
	"fmt"
)

const base64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

var base64Index = makeBase64Index()

func makeBase64Index() [256]int8 {
	var table [256]int8
	for i := range table {
		table[i] = -1
	}
	for i := 0; i < len(base64Alphabet); i++ {
		table[base64Alphabet[i]] = int8(i)
	}
	return table
}

var ErrBase64Length = errors.New("base64: length is not a multiple of 4")

var ErrBase64Padding = errors.New("base64: misplaced padding")

var ErrBase64TrailingBits = errors.New("base64: non-zero trailing bits")

// Base64ByteError reports a byte outside the standard alphabet.
type Base64ByteError struct {
	Index int
	Byte  byte
}

func (e *Base64ByteError) Error() string {
	return fmt.Sprintf("base64: invalid byte 0x%02x at index %d", e.Byte, e.Index)
}

// Base64Encode encodes data with the RFC 4648 standard alphabet and padding.
func Base64Encode(data []byte) string {
	out := make([]byte, 0, (len(data)+2)/3*4)
	for i := 0; i < len(data); i += 3 {
		chunk := data[i:min(i+3, len(data))]
		var triple uint32
		for j, b := range chunk {
			triple |= uint32(b) << (16 - 8*j)
		}
		groups := [4]uint32{triple >> 18, triple >> 12, triple >> 6, triple}
		for _, g := range groups[:len(chunk)+1] {
			out = append(out, base64Alphabet[g&0x3F])
		}
		for j := len(chunk); j < 3; j++ {
			out = append(out, '=')
		}
	}
	return string(out)
}

// Base64Decode is strict: canonical padded input over the standard alphabet
// only, with no whitespace and no non-zero trailing bits.
func Base64Decode(s string) ([]byte, error) {
	if len(s)%4 != 0 {
		return nil, ErrBase64Length
	}
	quads := len(s) / 4
	out := make([]byte, 0, quads*3)
	for q := range quads {
		last := q == quads-1
		var values [4]uint32
		padding := 0
		for i := range 4 {
			c := s[q*4+i]
			if c == '=' {
				if !last || i < 2 {
					return nil, ErrBase64Padding
				}
				padding++
				continue
			}
			if padding > 0 {
				return nil, ErrBase64Padding
			}
			v := base64Index[c]
			if v < 0 {
				return nil, &Base64ByteError{Index: q*4 + i, Byte: c}
			}
			values[i] = uint32(v)
		}

		var strayBits uint32
		switch padding {
		case 1:
			strayBits = values[2] & 0x03
		case 2:
			strayBits = values[1] & 0x0F
		}
		if strayBits != 0 {
			return nil, ErrBase64TrailingBits
		}

		triple := values[0]<<18 | values[1]<<12 | values[2]<<6 | values[3]
		decoded := [3]byte{byte(triple >> 16), byte(triple >> 8), byte(triple)}
		out = append(out, decoded[:3-padding]...)
	}
	return out, nil
}
