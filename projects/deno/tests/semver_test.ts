import { assertEquals, assertThrows } from "@std/assert";
import {
  compareSemver,
  formatSemver,
  parseSemver,
  type SemVer,
  SemVerError,
} from "../src/semver.ts";

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

Deno.test("semver parses and formats valid versions", async (t) => {
  for (const input of VALID) {
    await t.step(`parses ${JSON.stringify(input)}`, () => {
      assertEquals(formatSemver(parseSemver(input)), input);
    });
  }
});

Deno.test("semver rejects invalid versions", async (t) => {
  for (const input of INVALID) {
    await t.step(`rejects ${JSON.stringify(input)}`, () => {
      assertThrows(() => parseSemver(input), SemVerError);
    });
  }
});

Deno.test("semver extracts all fields", () => {
  assertEquals(parseSemver("1.2.3-rc.1+build.5"), {
    major: 1n,
    minor: 2n,
    patch: 3n,
    prerelease: ["rc", "1"],
    build: ["build", "5"],
  });
});

Deno.test("semver keeps dashes inside identifiers", () => {
  assertEquals(parseSemver("1.0.0-x-y-z.-").prerelease, ["x-y-z", "-"]);
});

Deno.test("semver keeps leading zeros in build identifiers", () => {
  assertEquals(parseSemver("1.0.0+001").build, ["001"]);
});

Deno.test("semver accepts the largest unsigned 64-bit component", () => {
  const version = parseSemver("18446744073709551615.0.0");
  assertEquals(version.major, 18446744073709551615n);
});

Deno.test("semver without prerelease or build has empty lists", () => {
  const version = parseSemver("0.0.0");
  assertEquals(version.prerelease, []);
  assertEquals(version.build, []);
});

Deno.test("semver ordering chain holds for all ordered pairs", () => {
  const versions = CHAIN.map((item) => parseSemver(item));
  for (const [i, a] of versions.entries()) {
    for (const [j, b] of versions.entries()) {
      assertEquals(compareSemver(a, b), Math.sign(i - j));
    }
  }
});

Deno.test("semver comparison cases", async (t) => {
  for (const [left, right, expected] of COMPARISONS) {
    await t.step(`compare(${left}, ${right})`, () => {
      const a = parseSemver(left);
      const b = parseSemver(right);
      assertEquals(compareSemver(a, b), expected);
      assertEquals(compareSemver(b, a), expected === 0 ? 0 : -expected);
    });
  }
});

Deno.test("semver compare is a consistent order on a grid", () => {
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
  assertEquals(violations, []);
});

Deno.test("semver build metadata never changes the result", () => {
  for (const a of grid()) {
    const stripped = parseSemver(formatSemver({ ...a, build: [] }));
    assertEquals(compareSemver(a, stripped), 0);
  }
});

Deno.test("semver format round trips through parse", () => {
  for (const version of grid()) {
    assertEquals(parseSemver(formatSemver(version)), version);
  }
});
