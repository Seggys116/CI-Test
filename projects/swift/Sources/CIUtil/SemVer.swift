public struct SemVerError: Error, Equatable, CustomStringConvertible, Sendable {
    public let reason: String

    public init(_ reason: String) {
        self.reason = reason
    }

    public var description: String {
        "invalid semantic version: \(reason)"
    }
}

/// a Semantic Versioning 2.0.0 version.
/// `==` is structural and includes build metadata; `compare` follows precedence and ignores it.
public struct SemVer: Equatable, Hashable, CustomStringConvertible, Sendable {
    public var major: UInt64
    public var minor: UInt64
    public var patch: UInt64
    public var prerelease: [String]
    public var build: [String]

    public init(
        major: UInt64,
        minor: UInt64,
        patch: UInt64,
        prerelease: [String] = [],
        build: [String] = []
    ) {
        self.major = major
        self.minor = minor
        self.patch = patch
        self.prerelease = prerelease
        self.build = build
    }

    public init(parsing text: String) throws {
        var rest = Array(text.utf8)[...]

        var buildPart: ArraySlice<UInt8>?
        if let plus = rest.firstIndex(of: UInt8(ascii: "+")) {
            buildPart = rest[(plus + 1)...]
            rest = rest[..<plus]
        }

        var prereleasePart: ArraySlice<UInt8>?
        if let dash = rest.firstIndex(of: UInt8(ascii: "-")) {
            prereleasePart = rest[(dash + 1)...]
            rest = rest[..<dash]
        }

        let fields = rest.split(separator: UInt8(ascii: "."), omittingEmptySubsequences: false)
        guard fields.count == 3 else {
            throw SemVerError("expected MAJOR.MINOR.PATCH")
        }

        self.init(
            major: try parseNumber(fields[0]),
            minor: try parseNumber(fields[1]),
            patch: try parseNumber(fields[2]),
            prerelease: try prereleasePart.map { try parseIdentifiers($0, allowLeadingZeros: false) } ?? [],
            build: try buildPart.map { try parseIdentifiers($0, allowLeadingZeros: true) } ?? []
        )
    }

    public static func parse(_ text: String) throws -> SemVer {
        try SemVer(parsing: text)
    }

    public var description: String {
        var result = "\(major).\(minor).\(patch)"
        if !prerelease.isEmpty {
            result += "-" + prerelease.joined(separator: ".")
        }
        if !build.isEmpty {
            result += "+" + build.joined(separator: ".")
        }
        return result
    }

    /// returns -1, 0 or 1 by semver precedence; build metadata is ignored.
    public static func compare(_ lhs: SemVer, _ rhs: SemVer) -> Int {
        for (a, b) in [(lhs.major, rhs.major), (lhs.minor, rhs.minor), (lhs.patch, rhs.patch)] {
            let result = order(a, b)
            if result != 0 {
                return result
            }
        }

        if lhs.prerelease.isEmpty || rhs.prerelease.isEmpty {
            if lhs.prerelease.isEmpty && rhs.prerelease.isEmpty {
                return 0
            }
            // a release outranks any of its prereleases
            return lhs.prerelease.isEmpty ? 1 : -1
        }

        for (a, b) in zip(lhs.prerelease, rhs.prerelease) {
            let result = compareIdentifiers(a, b)
            if result != 0 {
                return result
            }
        }
        return order(lhs.prerelease.count, rhs.prerelease.count)
    }

    public func compare(to other: SemVer) -> Int {
        SemVer.compare(self, other)
    }
}

private func order<T: Comparable>(_ a: T, _ b: T) -> Int {
    a < b ? -1 : (a > b ? 1 : 0)
}

private func isDigit(_ byte: UInt8) -> Bool {
    (0x30...0x39).contains(byte)
}

private func isIdentifierByte(_ byte: UInt8) -> Bool {
    switch byte {
    case 0x30...0x39, 0x41...0x5A, 0x61...0x7A, 0x2D:
        return true
    default:
        return false
    }
}

private func parseNumber(_ field: ArraySlice<UInt8>) throws -> UInt64 {
    guard !field.isEmpty, field.allSatisfy(isDigit) else {
        throw SemVerError("version fields must be non-negative integers")
    }
    guard field.count == 1 || field.first != 0x30 else {
        throw SemVerError("version fields must not have leading zeros")
    }

    var value: UInt64 = 0
    for byte in field {
        let (scaled, scaleOverflow) = value.multipliedReportingOverflow(by: 10)
        let (sum, addOverflow) = scaled.addingReportingOverflow(UInt64(byte - 0x30))
        guard !scaleOverflow, !addOverflow else {
            throw SemVerError("version field does not fit in 64 bits")
        }
        value = sum
    }
    return value
}

private func parseIdentifiers(_ part: ArraySlice<UInt8>, allowLeadingZeros: Bool) throws -> [String] {
    let identifiers = part.split(separator: UInt8(ascii: "."), omittingEmptySubsequences: false)
    return try identifiers.map { (identifier: ArraySlice<UInt8>) throws -> String in
        guard !identifier.isEmpty else {
            throw SemVerError("empty identifier")
        }
        guard identifier.allSatisfy(isIdentifierByte) else {
            throw SemVerError("identifiers may only contain [0-9A-Za-z-]")
        }
        if !allowLeadingZeros, identifier.count > 1, identifier.first == 0x30,
            identifier.allSatisfy(isDigit)
        {
            throw SemVerError("numeric identifiers must not have leading zeros")
        }
        return String(decoding: identifier, as: UTF8.self)
    }
}

private func isNumeric(_ identifier: String) -> Bool {
    !identifier.isEmpty && identifier.utf8.allSatisfy(isDigit)
}

private func compareIdentifiers(_ a: String, _ b: String) -> Int {
    switch (isNumeric(a), isNumeric(b)) {
    case (true, true):
        // numeric identifiers have no leading zeros, so length decides before digits do
        let lengthOrder = order(a.utf8.count, b.utf8.count)
        return lengthOrder != 0 ? lengthOrder : compareASCII(a, b)
    case (true, false):
        return -1
    case (false, true):
        return 1
    case (false, false):
        return compareASCII(a, b)
    }
}

private func compareASCII(_ a: String, _ b: String) -> Int {
    if a.utf8.elementsEqual(b.utf8) {
        return 0
    }
    return a.utf8.lexicographicallyPrecedes(b.utf8) ? -1 : 1
}
