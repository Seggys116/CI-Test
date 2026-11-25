const U64_MAX = 0xffffffffffffffffn;
const DIGITS = /^[0-9]+$/;
const LEADING_ZERO = /^0[0-9]+$/;
const IDENTIFIER = /^[0-9A-Za-z-]+$/;

export class SemVerError extends Error {}

export interface SemVer {
  readonly major: bigint;
  readonly minor: bigint;
  readonly patch: bigint;
  readonly prerelease: readonly string[];
  readonly build: readonly string[];
}

export type Ordering = -1 | 0 | 1;

function fail(reason: string): never {
  throw new SemVerError(`invalid semver: ${reason}`);
}

function parseNumber(part: string): bigint {
  if (!DIGITS.test(part)) {
    return fail(`invalid numeric component ${JSON.stringify(part)}`);
  }
  if (LEADING_ZERO.test(part)) {
    return fail(`leading zero in ${JSON.stringify(part)}`);
  }
  const value = BigInt(part);
  if (value > U64_MAX) {
    return fail(`number exceeds 64 bits: ${part}`);
  }
  return value;
}

function parseIdentifiers(list: string, prerelease: boolean): string[] {
  const identifiers = list.split(".");
  for (const identifier of identifiers) {
    if (!IDENTIFIER.test(identifier)) {
      fail(`invalid identifier ${JSON.stringify(identifier)}`);
    }
    if (prerelease && LEADING_ZERO.test(identifier)) {
      fail(`leading zero in identifier ${JSON.stringify(identifier)}`);
    }
  }
  return identifiers;
}

export function parseSemver(text: string): SemVer {
  const plus = text.indexOf("+");
  const head = plus === -1 ? text : text.slice(0, plus);
  const dash = head.indexOf("-");
  const core = dash === -1 ? head : head.slice(0, dash);
  const parts = core.split(".");
  if (parts.length !== 3) {
    return fail(`expected MAJOR.MINOR.PATCH in ${JSON.stringify(text)}`);
  }
  const major = parseNumber(parts[0]!);
  const minor = parseNumber(parts[1]!);
  const patch = parseNumber(parts[2]!);
  let prerelease: string[] = [];
  if (dash !== -1) {
    prerelease = parseIdentifiers(head.slice(dash + 1), true);
  }
  let build: string[] = [];
  if (plus !== -1) {
    build = parseIdentifiers(text.slice(plus + 1), false);
  }
  return { major, minor, patch, prerelease, build };
}

export function formatSemver(version: SemVer): string {
  let out = `${version.major}.${version.minor}.${version.patch}`;
  if (version.prerelease.length > 0) {
    out += `-${version.prerelease.join(".")}`;
  }
  if (version.build.length > 0) {
    out += `+${version.build.join(".")}`;
  }
  return out;
}

function sign<T extends bigint | number | string>(a: T, b: T): Ordering {
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

function compareIdentifiers(a: string, b: string): Ordering {
  const aNumeric = DIGITS.test(a);
  const bNumeric = DIGITS.test(b);
  if (aNumeric && bNumeric) {
    return sign(a.length, b.length) || sign(a, b);
  }
  if (aNumeric) {
    return -1;
  }
  if (bNumeric) {
    return 1;
  }
  return sign(a, b);
}

export function compareSemver(a: SemVer, b: SemVer): Ordering {
  const major = sign(a.major, b.major);
  if (major !== 0) {
    return major;
  }
  const minor = sign(a.minor, b.minor);
  if (minor !== 0) {
    return minor;
  }
  const patch = sign(a.patch, b.patch);
  if (patch !== 0) {
    return patch;
  }
  const left = a.prerelease;
  const right = b.prerelease;
  if (left.length === 0 || right.length === 0) {
    return sign(right.length, left.length);
  }
  const shared = Math.min(left.length, right.length);
  for (let i = 0; i < shared; i++) {
    const order = compareIdentifiers(left[i]!, right[i]!);
    if (order !== 0) {
      return order;
    }
  }
  return sign(left.length, right.length);
}
