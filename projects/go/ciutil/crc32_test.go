package ciutil_test

import (
	"bytes"
	"testing"

	"ciutil/ciutil"
)

func bitwiseCRC32(data []byte) uint32 {
	crc := uint32(0xFFFFFFFF)
	for _, b := range data {
		crc ^= uint32(b)
		for range 8 {
			if crc&1 == 1 {
				crc = (crc >> 1) ^ 0xEDB88320
			} else {
				crc >>= 1
			}
		}
	}
	return ^crc
}

func TestCRC32Vectors(t *testing.T) {
	tests := []struct {
		name string
		in   []byte
		want uint32
	}{
		{"empty", []byte(""), 0x00000000},
		{"a", []byte("a"), 0xe8b7be43},
		{"abc", []byte("abc"), 0x352441c2},
		{"digits", []byte("123456789"), 0xcbf43926},
		{"fox", []byte("The quick brown fox jumps over the lazy dog"), 0x414fa339},
		{"zeros", make([]byte, 32), 0x190a55ad},
		{"ones", bytes.Repeat([]byte{0xFF}, 32), 0xff6cab0b},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := ciutil.CRC32Checksum(tt.in); got != tt.want {
				t.Errorf("CRC32Checksum = %08x, want %08x", got, tt.want)
			}
		})
	}
}

func TestCRC32MatchesBitwiseReference(t *testing.T) {
	data := pseudoRandom(1000)
	for _, n := range []int{0, 1, 2, 3, 7, 8, 9, 255, 256, 1000} {
		if got, want := ciutil.CRC32Checksum(data[:n]), bitwiseCRC32(data[:n]); got != want {
			t.Errorf("length %d: got %08x, want %08x", n, got, want)
		}
	}
}

func TestCRC32ChunkedEqualsOneShotAtEverySplit(t *testing.T) {
	data := pseudoRandom(1000)
	whole := ciutil.CRC32Checksum(data)
	for split := 0; split <= len(data); split++ {
		var c ciutil.CRC32
		c.Update(data[:split])
		c.Update(data[split:])
		if got := c.Sum32(); got != whole {
			t.Fatalf("split at %d: got %08x, want %08x", split, got, whole)
		}
	}
}

func TestCRC32ManySmallChunks(t *testing.T) {
	data := pseudoRandom(1000)
	whole := ciutil.CRC32Checksum(data)
	for _, size := range []int{1, 2, 3, 7, 64, 999} {
		var c ciutil.CRC32
		for start := 0; start < len(data); start += size {
			c.Update(data[start:min(start+size, len(data))])
		}
		if got := c.Sum32(); got != whole {
			t.Errorf("chunk size %d: got %08x, want %08x", size, got, whole)
		}
	}
}

func TestCRC32SumDoesNotConsumeState(t *testing.T) {
	var c ciutil.CRC32
	c.Update([]byte("12345"))
	if got, want := c.Sum32(), ciutil.CRC32Checksum([]byte("12345")); got != want {
		t.Fatalf("partial sum = %08x, want %08x", got, want)
	}
	c.Update([]byte("6789"))
	if got := c.Sum32(); got != 0xcbf43926 {
		t.Errorf("final sum = %08x, want cbf43926", got)
	}
}
