import CIUtil
import Testing

struct CRC32Tests {
    static let vectors: [StringVector] = [
        StringVector(input: "", expected: "00000000"),
        StringVector(input: "a", expected: "e8b7be43"),
        StringVector(input: "abc", expected: "352441c2"),
        StringVector(input: "123456789", expected: "cbf43926"),
        StringVector(
            input: "The quick brown fox jumps over the lazy dog", expected: "414fa339"),
    ]

    @Test(arguments: CRC32Tests.vectors)
    func knownStrings(vector: StringVector) {
        #expect(hex8(CRC32.checksum(of: vector.input.utf8)) == vector.expected)
    }

    @Test func thirtyTwoZeroBytes() {
        #expect(hex8(CRC32.checksum(of: [UInt8](repeating: 0x00, count: 32))) == "190a55ad")
    }

    @Test func thirtyTwoFFBytes() {
        #expect(hex8(CRC32.checksum(of: [UInt8](repeating: 0xFF, count: 32))) == "ff6cab0b")
    }

    @Test func chunkedMatchesOneShotAtEverySplit() {
        let data = lcgBytes(count: 1000)
        let expected = CRC32.checksum(of: data)
        var failures: [Int] = []
        for split in 0...data.count {
            var hasher = CRC32()
            hasher.update(data[..<split])
            hasher.update(data[split...])
            if hasher.value != expected {
                failures.append(split)
            }
        }
        #expect(failures.isEmpty, "splits that disagree with one-shot: \(failures)")
    }

    @Test func manySmallUpdatesMatchOneShot() {
        let data = lcgBytes(count: 1000)
        var hasher = CRC32()
        for byte in data {
            hasher.update([byte])
        }
        #expect(hasher.value == CRC32.checksum(of: data))
    }

    @Test func valueDoesNotConsumeState() {
        var hasher = CRC32()
        hasher.update("1234".utf8)
        _ = hasher.value
        hasher.update("56789".utf8)
        #expect(hex8(hasher.value) == "cbf43926")
    }
}
