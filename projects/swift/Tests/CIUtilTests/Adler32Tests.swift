import CIUtil
import Testing

struct Adler32Tests {
    static let vectors: [StringVector] = [
        StringVector(input: "", expected: "00000001"),
        StringVector(input: "a", expected: "00620062"),
        StringVector(input: "abc", expected: "024d0127"),
        StringVector(input: "Wikipedia", expected: "11e60398"),
        StringVector(input: "123456789", expected: "091e01de"),
    ]

    @Test(arguments: Adler32Tests.vectors)
    func knownStrings(vector: StringVector) {
        #expect(hex8(Adler32.checksum(of: vector.input.utf8)) == vector.expected)
    }

    /// reference with a modulo after every byte, so it never relies on the block limit
    private func naiveAdler32(_ bytes: [UInt8]) -> UInt32 {
        var a: UInt64 = 1
        var b: UInt64 = 0
        for byte in bytes {
            a = (a + UInt64(byte)) % 65521
            b = (b + a) % 65521
        }
        return UInt32((b << 16) | a)
    }

    @Test func longInputOfFFBytesMatchesNaiveReference() {
        let data = [UInt8](repeating: 0xFF, count: 100_000)
        #expect(Adler32.checksum(of: data) == naiveAdler32(data))
    }

    @Test(arguments: [5551, 5552, 5553, 11104, 11105, 100_000])
    func lengthsAroundTheBlockLimitMatchNaiveReference(length: Int) {
        let data = [UInt8](repeating: 0xFF, count: length)
        #expect(Adler32.checksum(of: data) == naiveAdler32(data))
    }

    @Test func pseudoRandomInputMatchesNaiveReference() {
        let data = lcgBytes(count: 20_000)
        #expect(Adler32.checksum(of: data) == naiveAdler32(data))
    }

    @Test(arguments: [1, 7, 5551, 5552, 5553, 65_536])
    func chunkedLongInputMatchesOneShot(chunkSize: Int) {
        let data = [UInt8](repeating: 0xFF, count: 100_000)
        var hasher = Adler32()
        var start = 0
        while start < data.count {
            let end = min(start + chunkSize, data.count)
            hasher.update(data[start..<end])
            start = end
        }
        #expect(hasher.value == Adler32.checksum(of: data))
    }

    @Test func chunkedMatchesOneShotAtEverySplit() {
        let data = lcgBytes(count: 1000)
        let expected = Adler32.checksum(of: data)
        var failures: [Int] = []
        for split in 0...data.count {
            var hasher = Adler32()
            hasher.update(data[..<split])
            hasher.update(data[split...])
            if hasher.value != expected {
                failures.append(split)
            }
        }
        #expect(failures.isEmpty, "splits that disagree with one-shot: \(failures)")
    }
}
