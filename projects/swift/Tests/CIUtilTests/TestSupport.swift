/// deterministic bytes from the shared lcg: x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) & 0xFF, seed 42.
func lcgBytes(count: Int) -> [UInt8] {
    var state: UInt64 = 42
    var bytes = [UInt8]()
    bytes.reserveCapacity(count)
    for _ in 0..<count {
        state = (state &* 1_103_515_245 &+ 12345) & 0x7FFF_FFFF
        bytes.append(UInt8(truncatingIfNeeded: state >> 16))
    }
    return bytes
}

func hex8(_ value: UInt32) -> String {
    let digits = String(value, radix: 16)
    return String(repeating: "0", count: 8 - digits.count) + digits
}

struct StringVector: Sendable {
    let input: String
    let expected: String
}
