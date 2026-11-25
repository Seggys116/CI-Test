public struct Base64Error: Error, Equatable, CustomStringConvertible, Sendable {
    public let reason: String

    public init(_ reason: String) {
        self.reason = reason
    }

    public var description: String {
        "invalid base64: \(reason)"
    }
}

/// RFC 4648 standard alphabet with padding; the decoder is strict.
public enum Base64 {
    private static let alphabet: [UInt8] = Array(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/".utf8)

    private static let padding = UInt8(ascii: "=")

    // 0xFF marks bytes outside the alphabet
    private static let reverse: [UInt8] = {
        var table = [UInt8](repeating: 0xFF, count: 256)
        for (value, character) in Base64.alphabet.enumerated() {
            table[Int(character)] = UInt8(value)
        }
        return table
    }()

    public static func encode(_ bytes: some Sequence<UInt8>) -> String {
        let input = Array(bytes)
        var output = [UInt8]()
        output.reserveCapacity((input.count + 2) / 3 * 4)

        var index = 0
        while index + 3 <= input.count {
            let b0 = UInt32(input[index])
            let b1 = UInt32(input[index + 1])
            let b2 = UInt32(input[index + 2])
            let group = (b0 << 16) | (b1 << 8) | b2
            output.append(alphabet[Int((group >> 18) & 63)])
            output.append(alphabet[Int((group >> 12) & 63)])
            output.append(alphabet[Int((group >> 6) & 63)])
            output.append(alphabet[Int(group & 63)])
            index += 3
        }

        switch input.count - index {
        case 1:
            let group = UInt32(input[index]) << 16
            output.append(alphabet[Int((group >> 18) & 63)])
            output.append(alphabet[Int((group >> 12) & 63)])
            output.append(padding)
            output.append(padding)
        case 2:
            let group = (UInt32(input[index]) << 16) | (UInt32(input[index + 1]) << 8)
            output.append(alphabet[Int((group >> 18) & 63)])
            output.append(alphabet[Int((group >> 12) & 63)])
            output.append(alphabet[Int((group >> 6) & 63)])
            output.append(padding)
        default:
            break
        }
        return String(decoding: output, as: UTF8.self)
    }

    public static func decode(_ text: String) throws -> [UInt8] {
        let input = Array(text.utf8)
        guard input.count % 4 == 0 else {
            throw Base64Error("length is not a multiple of 4")
        }

        let blockCount = input.count / 4
        var output = [UInt8]()
        output.reserveCapacity(blockCount * 3)

        for block in 0..<blockCount {
            var group: UInt32 = 0
            var padCount = 0
            for offset in 0..<4 {
                let character = input[block * 4 + offset]
                var sextet: UInt32 = 0
                if character == padding {
                    // padding is only legal as the last one or two characters of the final block
                    guard block == blockCount - 1, offset >= 2 else {
                        throw Base64Error("misplaced padding")
                    }
                    padCount += 1
                } else {
                    guard padCount == 0 else {
                        throw Base64Error("data after padding")
                    }
                    let value = reverse[Int(character)]
                    guard value != 0xFF else {
                        throw Base64Error("byte 0x\(String(character, radix: 16)) is not in the alphabet")
                    }
                    sextet = UInt32(value)
                }
                group = (group << 6) | sextet
            }

            output.append(UInt8(truncatingIfNeeded: group >> 16))
            switch padCount {
            case 0:
                output.append(UInt8(truncatingIfNeeded: group >> 8))
                output.append(UInt8(truncatingIfNeeded: group))
            case 1:
                guard group & 0xFF == 0 else {
                    throw Base64Error("non-canonical trailing bits")
                }
                output.append(UInt8(truncatingIfNeeded: group >> 8))
            default:
                guard group & 0xFFFF == 0 else {
                    throw Base64Error("non-canonical trailing bits")
                }
            }
        }
        return output
    }
}
