/// CRC-32/IEEE (reflected, polynomial 0xEDB88320, init and xorout 0xFFFFFFFF).
public struct CRC32: Sendable {
    private static let table: [UInt32] = (0..<256).map { (index: UInt32) -> UInt32 in
        var crc = index
        for _ in 0..<8 {
            crc = (crc & 1) != 0 ? (0xEDB8_8320 ^ (crc >> 1)) : (crc >> 1)
        }
        return crc
    }

    private var state: UInt32

    public init() {
        state = 0xFFFF_FFFF
    }

    public mutating func update(_ bytes: some Sequence<UInt8>) {
        var crc = state
        for byte in bytes {
            crc = Self.table[Int((crc ^ UInt32(byte)) & 0xFF)] ^ (crc >> 8)
        }
        state = crc
    }

    /// checksum of everything passed to `update` so far; the hasher stays usable afterwards.
    public var value: UInt32 {
        state ^ 0xFFFF_FFFF
    }

    public static func checksum(of bytes: some Sequence<UInt8>) -> UInt32 {
        var hasher = CRC32()
        hasher.update(bytes)
        return hasher.value
    }
}
