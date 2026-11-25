private let usageText = """
    usage: ciutil <command> [arguments]
    commands:
      crc32 STRING
      adler32 STRING
      b64enc STRING
      b64dec BASE64
      roman NUMBER
      unroman ROMAN
      semver-cmp A B
      version

    """

private struct CLIError: Error, CustomStringConvertible {
    let description: String
}

/// runs one ciutil invocation. `arguments` excludes the program name. output is raw bytes appended
/// to `stdout` and `stderr`; the return value is the process exit code (0 ok, 2 error, 64 usage).
public func run(arguments: [String], stdout: inout [UInt8], stderr: inout [UInt8]) -> Int32 {
    guard let command = arguments.first else {
        append(usageText, to: &stderr)
        return 64
    }
    do {
        guard try execute(command, Array(arguments.dropFirst()), stdout: &stdout) else {
            append(usageText, to: &stderr)
            return 64
        }
        return 0
    } catch {
        append("ciutil: \(error)\n", to: &stderr)
        return 2
    }
}

private func execute(_ command: String, _ operands: [String], stdout: inout [UInt8]) throws -> Bool {
    switch (command, operands.count) {
    case ("version", 0):
        append("ciutil swift 1.0.0\n", to: &stdout)
    case ("crc32", 1):
        append(hex32(CRC32.checksum(of: operands[0].utf8)) + "\n", to: &stdout)
    case ("adler32", 1):
        append(hex32(Adler32.checksum(of: operands[0].utf8)) + "\n", to: &stdout)
    case ("b64enc", 1):
        append(Base64.encode(operands[0].utf8) + "\n", to: &stdout)
    case ("b64dec", 1):
        let decoded = try Base64.decode(operands[0])
        stdout.append(contentsOf: decoded)
    case ("roman", 1):
        let numeral = try Roman.toRoman(parseDecimal(operands[0]))
        append(numeral + "\n", to: &stdout)
    case ("unroman", 1):
        let number = try Roman.fromRoman(operands[0])
        append("\(number)\n", to: &stdout)
    case ("semver-cmp", 2):
        let lhs = try SemVer(parsing: operands[0])
        let rhs = try SemVer(parsing: operands[1])
        append("\(SemVer.compare(lhs, rhs))\n", to: &stdout)
    default:
        return false
    }
    return true
}

private func append(_ text: String, to buffer: inout [UInt8]) {
    buffer.append(contentsOf: text.utf8)
}

private func hex32(_ value: UInt32) -> String {
    let digits = String(value, radix: 16)
    return String(repeating: "0", count: 8 - digits.utf8.count) + digits
}

private func parseDecimal(_ text: String) throws -> Int {
    guard !text.isEmpty, text.utf8.allSatisfy({ (0x30...0x39).contains($0) }), let value = Int(text)
    else {
        throw CLIError(description: "not a non-negative decimal integer: \(text)")
    }
    return value
}
