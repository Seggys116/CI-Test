package ciutil_test

// pseudoRandom is the fixed LCG shared by every language of this repo.
func pseudoRandom(n int) []byte {
	out := make([]byte, n)
	x := uint64(42)
	for i := range out {
		x = (x*1103515245 + 12345) & 0x7FFFFFFF
		out[i] = byte((x >> 16) & 0xFF)
	}
	return out
}
