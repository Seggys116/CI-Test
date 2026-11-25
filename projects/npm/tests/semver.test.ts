import { describe, expect, test } from "vitest";
import {
  SemVerError,
  compareSemver,
  formatSemver,
  parseSemver,
  type SemVer,
} from "../src/semver.js";

const CHAIN = [
  "1.0.0-alpha",
  "1.0.0-alpha.1",
  "1.0.0-alpha.beta",
  "1.0.0-beta",
  "1.0.0-beta.2",
  "1.0.0-beta.11",
  "1.0.0-rc.1",
  "1.0.0",
  "2.0.0",
  "2.1.0",
  "2.1.1",
];

const VALID = [
  "0.0.0",
  "1.0.0",
  "1.0.0-0",
  "1.0.0-0a",
  "1.0.0-x-y-z.-",
  "1.0.0+001",
  "1.0.0-rc.1+build.5",
  "1.2.3-alpha.1.2.3+exp.sha.5114f85",
  "10.20.30",
  "1.0.0-alpha-beta",
  "1.0.0+a-b",
  "18446744073709551615.18446744073709551615.18446744073709551615",
];

const INVALID = [
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
  "1.0.0-alpha.01",
  "1.0.0-a..b",
  "1.0.0-.a",
  "1.0.0-a.",
  "1.0.0+a..b",
  "1.0.0+a_b",
  "1.0.0-a_b",
  "1.0.0+a+b",
  "-1.0.0",
  "+1.0.0",
  " 1.0.0",
  "1.0.0 ",
  "1.0.0\n",
  "1.0.0-alpha+",
  "a.b.c",
  "1.0.x",
  "1..0",
  "١.0.0",
  "1.0.0-é",
  "18446744073709551616.0.0",
  "0.18446744073709551616.0",
  "0.0.99999999999999999999999999",
];

const COMPARISONS: [string, string, number][] = [
  ["1.0.0+a", "1.0.0+b", 0],
  ["1.0.0+a", "1.0.0", 0],
  ["1.0.0-rc.1+x", "1.0.0-rc.1+y", 0],
  ["1.0.0-beta.2", "1.0.0-beta.11", -1],
  ["1.0.0-1", "1.0.0-a", -1],
  ["1.0.0-a", "1.0.0-B", 1],
  ["1.0.0-alpha", "1.0.0-alpha.1", -1],
  ["1.0.0-0a", "1.0.0-1", 1],
  ["1.0.0-99999999999999999999", "1.0.0-100000000000000000000", -1],
  ["1.9.0", "1.10.0", -1],
  ["2.0.0", "1.99.99", 1],
  ["18446744073709551615.0.0", "18446744073709551614.9.9", 1],
];

function grid(): SemVer[] {
  const cores = ["0.0.0", "1.0.0", "1.2.3"];
  const prereleases = [
    "",
    "-alpha",
    "-alpha.1",
    "-alpha.beta",
    "-beta",
    "-1",
    "-2",
    "-11",
    "-0a",
    "-rc.1",
  ];
  const builds = ["", "+x"];
  const versions: SemVer[] = [];
  for (const core of cores) {
    for (const prerelease of prereleases) {
      for (const build of builds) {
        versions.push(parseSemver(core + prerelease + build));
      }
    }
  }
  return versions;
}

describe("semver", () => {
  test.each(VALID)("parses and formats %j", (input) => {
    expect(formatSemver(parseSemver(input))).toBe(input);
  });

  test.each(INVALID)("rejects %j", (input) => {
    expect(() => parseSemver(input)).toThrow(SemVerError);
  });

  test("extracts all fields", () => {
    expect(parseSemver("1.2.3-rc.1+build.5")).toEqual({
      major: 1n,
      minor: 2n,
      patch: 3n,
      prerelease: ["rc", "1"],
      build: ["build", "5"],
    });
  });

  test("keeps dashes inside identifiers", () => {
    expect(parseSemver("1.0.0-x-y-z.-").prerelease).toEqual(["x-y-z", "-"]);
  });

  test("keeps leading zeros in build identifiers", () => {
    expect(parseSemver("1.0.0+001").build).toEqual(["001"]);
  });

  test("accepts the largest unsigned 64-bit component", () => {
    expect(parseSemver("18446744073709551615.0.0").major).toBe(
      18446744073709551615n,
    );
  });

  test("version without prerelease or build has empty lists", () => {
    const version = parseSemver("0.0.0");
    expect(version.prerelease).toEqual([]);
    expect(version.build).toEqual([]);
  });

  test("ordering chain holds for all ordered pairs", () => {
    const versions = CHAIN.map((item) => parseSemver(item));
    for (const [i, a] of versions.entries()) {
      for (const [j, b] of versions.entries()) {
        expect(compareSemver(a, b)).toBe(Math.sign(i - j));
      }
    }
  });

  test.each(COMPARISONS)("compare(%s, %s) is %i", (left, right, expected) => {
    const a = parseSemver(left);
    const b = parseSemver(right);
    expect(compareSemver(a, b)).toBe(expected);
    expect(compareSemver(b, a)).toBe(expected === 0 ? 0 : -expected);
  });

  test("compare is reflexive, antisymmetric and transitive on a grid", () => {
    const versions = grid();
    const violations: string[] = [];
    for (const a of versions) {
      if (compareSemver(a, a) !== 0) {
        violations.push(`reflexive ${formatSemver(a)}`);
      }
      for (const b of versions) {
        const ab = compareSemver(a, b);
        if (ab + compareSemver(b, a) !== 0) {
          violations.push(`antisymmetric ${formatSemver(a)}`);
        }
        for (const c of versions) {
          const bc = compareSemver(b, c);
          if (ab <= 0 && bc <= 0 && compareSemver(a, c) > 0) {
            violations.push(`transitive ${formatSemver(a)}`);
          }
        }
      }
    }
    expect(violations).toEqual([]);
  });

  test("build metadata never changes the result", () => {
    const versions = grid();
    for (const a of versions) {
      const stripped = parseSemver(formatSemver({ ...a, build: [] }));
      expect(compareSemver(a, stripped)).toBe(0);
    }
  });

  test("format round trips through parse for the whole grid", () => {
    for (const version of grid()) {
      expect(parseSemver(formatSemver(version))).toEqual(version);
    }
  });
});
