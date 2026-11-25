package ciutil

const (
	adlerMod  = 65521
	adlerNMax = 5552
)

// Adler32 computes the RFC 1950 checksum incrementally; create it with NewAdler32.
type Adler32 struct {
	a, b uint32
}

func NewAdler32() *Adler32 {
	return &Adler32{a: 1}
}

func (d *Adler32) Update(p []byte) {
	a, b := d.a, d.b
	for len(p) > 0 {
		n := min(len(p), adlerNMax)
		for _, x := range p[:n] {
			a += uint32(x)
			b += a
		}
		a %= adlerMod
		b %= adlerMod
		p = p[n:]
	}
	d.a, d.b = a, b
}

func (d *Adler32) Sum32() uint32 {
	return d.b<<16 | d.a
}

func Adler32Checksum(data []byte) uint32 {
	d := NewAdler32()
	d.Update(data)
	return d.Sum32()
}
