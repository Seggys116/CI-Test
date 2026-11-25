package ciutil

const crc32Poly = 0xEDB88320

var crc32Table = makeCRC32Table()

func makeCRC32Table() [256]uint32 {
	var table [256]uint32
	for i := range table {
		c := uint32(i)
		for range 8 {
			if c&1 == 1 {
				c = crc32Poly ^ (c >> 1)
			} else {
				c >>= 1
			}
		}
		table[i] = c
	}
	return table
}

// CRC32 computes CRC-32/IEEE incrementally; the zero value is ready to use.
// sum holds the finalized checksum so that the zero value means empty input.
type CRC32 struct {
	sum uint32
}

func (c *CRC32) Update(p []byte) {
	crc := ^c.sum
	for _, b := range p {
		crc = crc32Table[byte(crc)^b] ^ (crc >> 8)
	}
	c.sum = ^crc
}

func (c *CRC32) Sum32() uint32 {
	return c.sum
}

func CRC32Checksum(data []byte) uint32 {
	var c CRC32
	c.Update(data)
	return c.Sum32()
}
