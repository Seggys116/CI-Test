/// Adler-32 as defined by RFC 1950.
public struct Adler32: Sendable {
    private static let modulus: UInt32 = 65521
    // largest block length for which the sums cannot overflow 32 bits before reduction
    private static let blockLimit = 5552

    private var low: UInt32
    private var high: UInt32

    public init() {
        low = 1
        high = 0
    }

    public mutating func update(_ bytes: some Sequence<UInt8>) {
        var a = low
        var b = high
        var pending = 0
        for byte in bytes {
            a &+= UInt32(byte)
            b &+= a
            pending += 1
            if pending == Self.blockLimit {
                a %= Self.modulus
                b %= Self.modulus
                pending = 0
            }
        }
        low = a % Self.modulus
        high = b % Self.modulus
    }

    /// checksum of everything passed to `update` so far; the hasher stays usable afterwards.
    public var value: UInt32 {
        (high << 16) | low
    }

    public static func checksum(of bytes: some Sequence<UInt8>) -> UInt32 {
        var hasher = Adler32()
        hasher.update(bytes)
        return hasher.value
    }
}
