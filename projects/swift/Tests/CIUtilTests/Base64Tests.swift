import CIUtil
import Testing

struct Base64Tests {
    static let vectors: [StringVector] = [
        StringVector(input: "", expected: ""),
        StringVector(input: "f", expected: "Zg=="),
        StringVector(input: "fo", expected: "Zm8="),
        StringVector(input: "foo", expected: "Zm9v"),
        StringVector(input: "foob", expected: "Zm9vYg=="),
        StringVector(input: "fooba", expected: "Zm9vYmE="),
        StringVector(input: "foobar", expected: "Zm9vYmFy"),
    ]

    static let invalidInputs: [String] = [
        "Z",
        "Zg",
        "Zg=",
        "Zm9vY",
        "Zg==Zg==",
        "Zg=a",
        "=Zg=",
        "Z===",
        "====",
        "Zg===",
        "Zm9vYg===",
        "Zm9=Zm9v",
        "Zm=v",
        "Zm9v-A==",
        "Zm9v_A==",
        "ZmZ-",
        "Zm9_",
        "Zm9v\n",
        "Zm9v ",
        " Zm9v",
        "Zm 9",
        "Zm9v\r\n",
        "Zm\u{00E9}",
        "Zm9\u{00E9}",
        "Zm9\u{1F600}",
        "Zh==",
        "Zm9=",
        "Zg=\u{0}",
    ]

    @Test(arguments: Base64Tests.vectors)
    func encodesKnownStrings(vector: StringVector) {
        #expect(Base64.encode(vector.input.utf8) == vector.expected)
    }

    @Test(arguments: Base64Tests.vectors)
    func decodesKnownStrings(vector: StringVector) throws {
        #expect(try Base64.decode(vector.expected) == Array(vector.input.utf8))
    }

    @Test func encodesNonTextBytes() {
        #expect(Base64.encode([0x00, 0xFF, 0xFE] as [UInt8]) == "AP/+")
    }

    @Test func decodesNonTextBytes() throws {
        #expect(try Base64.decode("AP/+") == [0x00, 0xFF, 0xFE])
    }

    @Test(arguments: Base64Tests.invalidInputs)
    func rejectsInvalidInput(text: String) {
        #expect(throws: Base64Error.self) {
            try Base64.decode(text)
        }
    }

    @Test func roundTripsEveryLengthUpTo300() throws {
        let data = lcgBytes(count: 300)
        var failures: [Int] = []
        for length in 0...300 {
            let original = Array(data[..<length])
            let decoded = try Base64.decode(Base64.encode(original))
            if decoded != original {
                failures.append(length)
            }
        }
        #expect(failures.isEmpty, "lengths that did not round trip: \(failures)")
    }

    @Test func encodedLengthIsPaddedToAMultipleOfFour() {
        let data = lcgBytes(count: 300)
        for length in 0...300 {
            let encoded = Base64.encode(data[..<length])
            #expect(encoded.utf8.count == (length + 2) / 3 * 4)
        }
    }

    @Test func roundTripsEveryByteValue() throws {
        let all = (0...255).map { UInt8($0) }
        #expect(try Base64.decode(Base64.encode(all)) == all)
    }
}
