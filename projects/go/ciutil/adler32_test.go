package ciutil_test

import (
	"bytes"
	"testing"

	"ciutil/ciutil"
)

const (
	testAdlerMod  = 65521
	testAdlerNMax = 5552
)

func naiveAdler32(data []byte) uint32 {
	a, b := uint32(1), uint32(0)
	for _, x := range data {
		a = (a + uint32(x)) % testAdlerMod
		b = (b + a) % testAdlerMod
	}
	return b<<16 | a
}

func TestAdler32Vectors(t *testing.T) {
	tests := []struct {
		name string
		in   string
		want uint32
	}{
		{"empty", "", 0x00000001},
		{"a", "a", 0x00620062},
		{"abc", "abc", 0x024d0127},
		{"wikipedia", "Wikipedia", 0x11e60398},
		{"digits", "123456789", 0x091e01de},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := ciutil.Adler32Checksum([]byte(tt.in)); got != tt.want {
				t.Errorf("Adler32Checksum = %08x, want %08x", got, tt.want)
			}
		})
	}
}

func TestAdler32LongRunOfFFMatchesNaiveReference(t *testing.T) {
	data := bytes.Repeat([]byte{0xFF}, 100000)
	if got, want := ciutil.Adler32Checksum(data), naiveAdler32(data); got != want {
		t.Errorf("got %08x, want %08x", got, want)
	}
}

func TestAdler32LengthsAroundNMax(t *testing.T) {
	random := pseudoRandom(3*testAdlerNMax + 5)
	ones := bytes.Repeat([]byte{0xFF}, 2*testAdlerNMax+1)
	lengths := []int{
		testAdlerNMax - 1,
		testAdlerNMax,
		testAdlerNMax + 1,
		2*testAdlerNMax - 1,
		2 * testAdlerNMax,
		2*testAdlerNMax + 1,
		3 * testAdlerNMax,
		3*testAdlerNMax + 5,
	}
	for _, n := range lengths {
		if got, want := ciutil.Adler32Checksum(random[:n]), naiveAdler32(random[:n]); got != want {
			t.Errorf("random length %d: got %08x, want %08x", n, got, want)
		}
		if n <= len(ones) {
			if got, want := ciutil.Adler32Checksum(ones[:n]), naiveAdler32(ones[:n]); got != want {
				t.Errorf("0xFF length %d: got %08x, want %08x", n, got, want)
			}
		}
	}
}

func TestAdler32ChunkedEqualsOneShotAtEverySplit(t *testing.T) {
	data := pseudoRandom(1000)
	whole := ciutil.Adler32Checksum(data)
	if want := naiveAdler32(data); whole != want {
		t.Fatalf("one-shot = %08x, naive = %08x", whole, want)
	}
	for split := 0; split <= len(data); split++ {
		d := ciutil.NewAdler32()
		d.Update(data[:split])
		d.Update(data[split:])
		if got := d.Sum32(); got != whole {
			t.Fatalf("split at %d: got %08x, want %08x", split, got, whole)
		}
	}
}

func TestAdler32ChunkedLongInput(t *testing.T) {
	data := bytes.Repeat([]byte{0xFF}, 100000)
	whole := ciutil.Adler32Checksum(data)
	for _, size := range []int{1, 7, 4096, testAdlerNMax - 1, testAdlerNMax, testAdlerNMax + 1, 99999} {
		d := ciutil.NewAdler32()
		for start := 0; start < len(data); start += size {
			d.Update(data[start:min(start+size, len(data))])
		}
		if got := d.Sum32(); got != whole {
			t.Errorf("chunk size %d: got %08x, want %08x", size, got, whole)
		}
	}
}

func TestAdler32SumDoesNotConsumeState(t *testing.T) {
	d := ciutil.NewAdler32()
	d.Update([]byte("Wiki"))
	if got, want := d.Sum32(), ciutil.Adler32Checksum([]byte("Wiki")); got != want {
		t.Fatalf("partial sum = %08x, want %08x", got, want)
	}
	d.Update([]byte("pedia"))
	if got := d.Sum32(); got != 0x11e60398 {
		t.Errorf("final sum = %08x, want 11e60398", got)
	}
}
