import CIUtil
import Testing

struct RomanTests {
    struct Pair: Sendable {
        let number: Int
        let numeral: String
    }

    static let pairs: [Pair] = [
        Pair(number: 1, numeral: "I"),
        Pair(number: 4, numeral: "IV"),
        Pair(number: 9, numeral: "IX"),
        Pair(number: 14, numeral: "XIV"),
        Pair(number: 40, numeral: "XL"),
        Pair(number: 90, numeral: "XC"),
        Pair(number: 400, numeral: "CD"),
        Pair(number: 1994, numeral: "MCMXCIV"),
        Pair(number: 2024, numeral: "MMXXIV"),
        Pair(number: 3999, numeral: "MMMCMXCIX"),
    ]

    static let invalidNumerals: [String] = [
        "",
        "IIII",
        "VX",
        "IC",
        "XM",
        "VV",
        "IIX",
        "xiv",
        "iv",
        "IIV",
        "VIV",
        "XXXX",
        "LL",
        "DD",
        "CCCC",
        "MMMM",
        "MMMMCMXCIX",
        "IL",
        "XD",
        "VL",
        "CCM",
        "IXI",
        "IVI",
        "XIIII",
        "A",
        "I ",
        " I",
        "I\n",
        "X-I",
        "0",
        "\u{2160}",
    ]

    @Test(arguments: RomanTests.pairs)
    func convertsToRoman(pair: Pair) throws {
        #expect(try Roman.toRoman(pair.number) == pair.numeral)
    }

    @Test(arguments: RomanTests.pairs)
    func convertsFromRoman(pair: Pair) throws {
        #expect(try Roman.fromRoman(pair.numeral) == pair.number)
    }

    @Test(arguments: [0, -1, -3999, 4000, 4001, Int.max, Int.min])
    func rejectsOutOfRangeNumbers(number: Int) {
        #expect(throws: RomanError.self) {
            try Roman.toRoman(number)
        }
    }

    @Test(arguments: RomanTests.invalidNumerals)
    func rejectsNonCanonicalNumerals(text: String) {
        #expect(throws: RomanError.self) {
            try Roman.fromRoman(text)
        }
    }

    @Test func everyNumberRoundTrips() throws {
        var failures: [Int] = []
        for number in 1...3999 {
            let numeral = try Roman.toRoman(number)
            if try Roman.fromRoman(numeral) != number {
                failures.append(number)
            }
        }
        #expect(failures.isEmpty, "numbers that did not round trip: \(failures)")
    }

    @Test func numeralsAreDistinctAndUppercase() throws {
        var seen = Set<String>()
        for number in 1...3999 {
            let numeral = try Roman.toRoman(number)
            #expect(numeral.utf8.allSatisfy { "IVXLCDM".utf8.contains($0) })
            seen.insert(numeral)
        }
        #expect(seen.count == 3999)
    }
}
