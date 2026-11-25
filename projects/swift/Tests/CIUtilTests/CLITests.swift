import CIUtil
import Testing

struct CLITests {
    struct Outcome {
        let status: Int32
        let stdout: [UInt8]
        let stderr: [UInt8]

        var stdoutText: String { String(decoding: stdout, as: UTF8.self) }
    }

    struct Case: Sendable {
        let arguments: [String]
        let stdout: String
    }

    static let successes: [Case] = [
        Case(arguments: ["crc32", ""], stdout: "00000000\n"),
        Case(arguments: ["crc32", "a"], stdout: "e8b7be43\n"),
        Case(arguments: ["crc32", "123456789"], stdout: "cbf43926\n"),
        Case(
            arguments: ["crc32", "The quick brown fox jumps over the lazy dog"],
            stdout: "414fa339\n"),
        Case(arguments: ["adler32", ""], stdout: "00000001\n"),
        Case(arguments: ["adler32", "Wikipedia"], stdout: "11e60398\n"),
        Case(arguments: ["adler32", "123456789"], stdout: "091e01de\n"),
        Case(arguments: ["b64enc", ""], stdout: "\n"),
        Case(arguments: ["b64enc", "foobar"], stdout: "Zm9vYmFy\n"),
        Case(arguments: ["b64enc", "fooba"], stdout: "Zm9vYmE=\n"),
        Case(arguments: ["b64enc", "f"], stdout: "Zg==\n"),
        Case(arguments: ["b64dec", ""], stdout: ""),
        Case(arguments: ["b64dec", "Zm9vYmFy"], stdout: "foobar"),
        Case(arguments: ["b64dec", "Zm9vYg=="], stdout: "foob"),
        Case(arguments: ["roman", "1"], stdout: "I\n"),
        Case(arguments: ["roman", "1994"], stdout: "MCMXCIV\n"),
        Case(arguments: ["roman", "3999"], stdout: "MMMCMXCIX\n"),
        Case(arguments: ["unroman", "XIV"], stdout: "14\n"),
        Case(arguments: ["unroman", "MMXXIV"], stdout: "2024\n"),
        Case(arguments: ["semver-cmp", "1.0.0-alpha", "1.0.0-alpha.1"], stdout: "-1\n"),
        Case(arguments: ["semver-cmp", "1.0.0+a", "1.0.0+b"], stdout: "0\n"),
        Case(arguments: ["semver-cmp", "2.1.1", "2.1.0"], stdout: "1\n"),
        Case(arguments: ["version"], stdout: "ciutil swift 1.0.0\n"),
    ]

    static let errors: [[String]] = [
        ["roman", "0"],
        ["roman", "4000"],
        ["roman", "-1"],
        ["roman", "abc"],
        ["roman", ""],
        ["roman", "+5"],
        ["unroman", "IIII"],
        ["unroman", ""],
        ["unroman", "xiv"],
        ["b64dec", "Zg="],
        ["b64dec", "Zh=="],
        ["b64dec", "Zm9v\n"],
        ["semver-cmp", "1.0", "1.0.0"],
        ["semver-cmp", "1.0.0", "v1.0.0"],
    ]

    static let usageErrors: [[String]] = [
        [],
        [""],
        ["bogus"],
        ["version", "extra"],
        ["crc32"],
        ["crc32", "a", "b"],
        ["adler32"],
        ["b64enc"],
        ["b64enc", "a", "b"],
        ["b64dec"],
        ["roman"],
        ["roman", "1", "2"],
        ["unroman"],
        ["semver-cmp"],
        ["semver-cmp", "1.0.0"],
        ["semver-cmp", "1.0.0", "1.0.0", "1.0.0"],
        ["CRC32", "a"],
    ]

    private func invoke(_ arguments: [String]) -> Outcome {
        var out = [UInt8]()
        var err = [UInt8]()
        let status = run(arguments: arguments, stdout: &out, stderr: &err)
        return Outcome(status: status, stdout: out, stderr: err)
    }

    @Test(arguments: CLITests.successes)
    func succeeds(testCase: Case) {
        let result = invoke(testCase.arguments)
        #expect(result.status == 0)
        #expect(result.stdoutText == testCase.stdout)
        #expect(result.stderr.isEmpty)
    }

    @Test(arguments: CLITests.errors)
    func reportsErrorsWithExitCodeTwo(arguments: [String]) {
        let result = invoke(arguments)
        #expect(result.status == 2)
        #expect(result.stdout.isEmpty)
        #expect(!result.stderr.isEmpty)
    }

    @Test(arguments: CLITests.usageErrors)
    func reportsUsageWithExitCode64(arguments: [String]) {
        let result = invoke(arguments)
        #expect(result.status == 64)
        #expect(result.stdout.isEmpty)
        #expect(!result.stderr.isEmpty)
    }

    @Test func decodedBytesAreWrittenRawWithoutNewline() {
        let result = invoke(["b64dec", "AP/+"])
        #expect(result.status == 0)
        #expect(result.stdout == [0x00, 0xFF, 0xFE])
    }

    @Test func nonAsciiArgumentsAreHashedAsUtf8() {
        // "\u{00E9}" is the two bytes c3 a9
        let viaCli = invoke(["crc32", "\u{00E9}"])
        let direct = hex8(CRC32.checksum(of: [0xC3, 0xA9] as [UInt8])) + "\n"
        #expect(viaCli.stdoutText == direct)
        #expect(invoke(["b64enc", "\u{00E9}"]).stdoutText == "w6k=\n")
    }

    @Test func outputUsesNewlinesOnly() {
        let result = invoke(["crc32", "abc"])
        #expect(!result.stdout.contains(0x0D))
        #expect(result.stdout.last == 0x0A)
    }
}
