import CIUtil
import Testing

struct SemVerTests {
    static let rejected: [String] = [
        "",
        "1",
        "1.0",
        "1.0.0.0",
        "v1.0.0",
        "01.0.0",
        "1.00.0",
        "1.0.00",
        "1.0.0-",
        "1.0.0+",
        "1.0.0-01",
        "1.0.0-a..b",
        "1.0.0-a_b",
        "-1.0.0",
        " 1.0.0",
        "1.0.0 ",
        "1.0.0-alpha+",
        "1.0.0-alpha.",
        "1.0.0-.alpha",
        "1.0.0+a..b",
        "1.0.0+a_b",
        "1.0.0+a+b",
        "1.0.0\n",
        "1..0",
        "1.0.",
        ".1.0",
        "a.b.c",
        "1.0.x",
        "1.0.0-alpha.01",
        "+1.0.0",
        "1.0.0-\u{00E9}",
        "18446744073709551616.0.0",
        "1.18446744073709551616.0",
        "1.0.99999999999999999999",
    ]

    static let accepted: [String] = [
        "0.0.0",
        "1.0.0",
        "1.0.0-0",
        "1.0.0-0a",
        "1.0.0-x-y-z.-",
        "1.0.0+001",
        "1.0.0-rc.1+build.5",
        "1.2.3-alpha.1.beta-2+exp.sha.5114f85",
        "10.20.30",
        "1.0.0-alpha-a.b-c-somethinglong+build.1-aef.1-its-okay",
        "18446744073709551615.18446744073709551615.18446744073709551615",
    ]

    static let chain: [String] = [
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
    ]

    static let coreChain: [String] = ["1.0.0", "2.0.0", "2.1.0", "2.1.1"]

    @Test(arguments: SemVerTests.rejected)
    func rejectsInvalidVersions(text: String) {
        #expect(throws: SemVerError.self) {
            try SemVer(parsing: text)
        }
    }

    @Test(arguments: SemVerTests.accepted)
    func acceptsValidVersionsAndRoundTripsThroughDescription(text: String) throws {
        let version = try SemVer(parsing: text)
        #expect(version.description == text)
        #expect(try SemVer.parse(version.description) == version)
    }

    @Test func parsesAllComponents() throws {
        let version = try SemVer(parsing: "1.2.3-rc.1+build.5")
        #expect(version.major == 1)
        #expect(version.minor == 2)
        #expect(version.patch == 3)
        #expect(version.prerelease == ["rc", "1"])
        #expect(version.build == ["build", "5"])
    }

    @Test func parsesPlainVersion() throws {
        let version = try SemVer(parsing: "0.0.0")
        #expect(version == SemVer(major: 0, minor: 0, patch: 0))
        #expect(version.prerelease.isEmpty)
        #expect(version.build.isEmpty)
    }

    @Test func parsesLargestSixtyFourBitFields() throws {
        let version = try SemVer(parsing: "18446744073709551615.0.0")
        #expect(version.major == UInt64.max)
    }

    @Test func hyphensInsideIdentifiersAreKept() throws {
        let version = try SemVer(parsing: "1.0.0-x-y-z.-")
        #expect(version.prerelease == ["x-y-z", "-"])
    }

    @Test func orderingChainHoldsForAllOrderedPairs() throws {
        let versions = try SemVerTests.chain.map { try SemVer(parsing: $0) }
        var failures: [String] = []
        for i in versions.indices {
            for j in versions.indices where i < j {
                if SemVer.compare(versions[i], versions[j]) != -1 {
                    failures.append("\(SemVerTests.chain[i]) < \(SemVerTests.chain[j])")
                }
                if SemVer.compare(versions[j], versions[i]) != 1 {
                    failures.append("\(SemVerTests.chain[j]) > \(SemVerTests.chain[i])")
                }
            }
            if SemVer.compare(versions[i], versions[i]) != 0 {
                failures.append("\(SemVerTests.chain[i]) == itself")
            }
        }
        #expect(failures.isEmpty, "violated: \(failures)")
    }

    @Test func coreOrderingHoldsForAllOrderedPairs() throws {
        let versions = try SemVerTests.coreChain.map { try SemVer(parsing: $0) }
        for i in versions.indices {
            for j in versions.indices where i < j {
                #expect(SemVer.compare(versions[i], versions[j]) == -1)
                #expect(SemVer.compare(versions[j], versions[i]) == 1)
            }
        }
    }

    @Test func compareIsAntisymmetricOverAllSamples() throws {
        let samples = SemVerTests.chain + SemVerTests.coreChain + SemVerTests.accepted
        let versions = try samples.map { try SemVer(parsing: $0) }
        var failures: [String] = []
        for a in versions {
            for b in versions where SemVer.compare(a, b) != -SemVer.compare(b, a) {
                failures.append("\(a) vs \(b)")
            }
        }
        #expect(failures.isEmpty, "not antisymmetric: \(failures)")
    }

    @Test func buildMetadataIsIgnored() throws {
        let a = try SemVer(parsing: "1.0.0+a")
        let b = try SemVer(parsing: "1.0.0+b")
        #expect(SemVer.compare(a, b) == 0)
        #expect(a.compare(to: b) == 0)
        let c = try SemVer(parsing: "1.0.0-rc.1+x")
        let d = try SemVer(parsing: "1.0.0-rc.1")
        #expect(SemVer.compare(c, d) == 0)
    }

    @Test func numericIdentifiersCompareNumerically() throws {
        let two = try SemVer(parsing: "1.0.0-beta.2")
        let eleven = try SemVer(parsing: "1.0.0-beta.11")
        #expect(SemVer.compare(two, eleven) == -1)
    }

    @Test func hugeNumericIdentifiersCompareWithoutOverflow() throws {
        let smaller = try SemVer(parsing: "1.0.0-99999999999999999999999999")
        let larger = try SemVer(parsing: "1.0.0-100000000000000000000000000")
        #expect(SemVer.compare(smaller, larger) == -1)
        #expect(SemVer.compare(larger, smaller) == 1)
    }

    @Test func numericIdentifiersSortBeforeAlphanumeric() throws {
        let numeric = try SemVer(parsing: "1.0.0-99999")
        let alphanumeric = try SemVer(parsing: "1.0.0-a")
        #expect(SemVer.compare(numeric, alphanumeric) == -1)
        #expect(SemVer.compare(alphanumeric, numeric) == 1)
    }

    @Test func alphanumericIdentifiersCompareInAsciiOrder() throws {
        let upper = try SemVer(parsing: "1.0.0-Z")
        let lower = try SemVer(parsing: "1.0.0-a")
        #expect(SemVer.compare(upper, lower) == -1)
        let hyphen = try SemVer(parsing: "1.0.0-a-")
        let letter = try SemVer(parsing: "1.0.0-aa")
        #expect(SemVer.compare(hyphen, letter) == -1)
    }

    @Test func longerPrereleaseListWinsWhenPrefixIsEqual() throws {
        let short = try SemVer(parsing: "1.0.0-alpha")
        let long = try SemVer(parsing: "1.0.0-alpha.0")
        #expect(SemVer.compare(short, long) == -1)
    }

    @Test func releaseOutranksPrerelease() throws {
        let release = try SemVer(parsing: "2.0.0")
        let prerelease = try SemVer(parsing: "2.0.0-rc.1")
        #expect(SemVer.compare(release, prerelease) == 1)
    }

    @Test func prereleaseOfALaterVersionOutranksEarlierRelease() throws {
        let earlier = try SemVer(parsing: "1.9.9")
        let later = try SemVer(parsing: "2.0.0-alpha")
        #expect(SemVer.compare(earlier, later) == -1)
    }
}
