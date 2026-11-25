public struct RomanError: Error, Equatable, CustomStringConvertible, Sendable {
    public let reason: String

    public init(_ reason: String) {
        self.reason = reason
    }

    public var description: String {
        reason
    }
}

/// roman numerals for 1...3999; parsing accepts only the canonical spelling.
public enum Roman {
    private struct Entry: Sendable {
        let value: Int
        let text: String
    }

    private static let entries: [Entry] = [
        Entry(value: 1000, text: "M"),
        Entry(value: 900, text: "CM"),
        Entry(value: 500, text: "D"),
        Entry(value: 400, text: "CD"),
        Entry(value: 100, text: "C"),
        Entry(value: 90, text: "XC"),
        Entry(value: 50, text: "L"),
        Entry(value: 40, text: "XL"),
        Entry(value: 10, text: "X"),
        Entry(value: 9, text: "IX"),
        Entry(value: 5, text: "V"),
        Entry(value: 4, text: "IV"),
        Entry(value: 1, text: "I"),
    ]

    public static func toRoman(_ number: Int) throws -> String {
        guard (1...3999).contains(number) else {
            throw RomanError("\(number) is outside the range 1 to 3999")
        }
        var remaining = number
        var result = ""
        for entry in entries {
            while remaining >= entry.value {
                result += entry.text
                remaining -= entry.value
            }
        }
        return result
    }

    public static func fromRoman(_ text: String) throws -> Int {
        let bytes = Array(text.utf8)
        guard !bytes.isEmpty else {
            throw RomanError("empty roman numeral")
        }

        var values = [Int]()
        values.reserveCapacity(bytes.count)
        for byte in bytes {
            guard let value = symbolValue(byte) else {
                throw RomanError("invalid roman numeral: \(text)")
            }
            values.append(value)
        }

        var total = 0
        for (index, value) in values.enumerated() {
            if index + 1 < values.count, value < values[index + 1] {
                total -= value
            } else {
                total += value
            }
        }

        // the canonical spelling of a number is unique, so re-encoding rejects every other form
        guard (1...3999).contains(total), try toRoman(total) == text else {
            throw RomanError("invalid roman numeral: \(text)")
        }
        return total
    }

    private static func symbolValue(_ byte: UInt8) -> Int? {
        switch byte {
        case UInt8(ascii: "I"): return 1
        case UInt8(ascii: "V"): return 5
        case UInt8(ascii: "X"): return 10
        case UInt8(ascii: "L"): return 50
        case UInt8(ascii: "C"): return 100
        case UInt8(ascii: "D"): return 500
        case UInt8(ascii: "M"): return 1000
        default: return nil
        }
    }
}
