import { describe, expect, test } from "vitest";
import { crc32 } from "../src/crc32.js";
import { hex32, runCli, text } from "./helpers.js";

const OK_CASES: [string[], string][] = [
  [["version"], "ciutil npm 1.0.0\n"],
  [["crc32", "123456789"], "cbf43926\n"],
  [["crc32", ""], "00000000\n"],
  [["crc32", "a"], "e8b7be43\n"],
  [["crc32", "The quick brown fox jumps over the lazy dog"], "414fa339\n"],
  [["adler32", "Wikipedia"], "11e60398\n"],
  [["adler32", ""], "00000001\n"],
  [["adler32", "123456789"], "091e01de\n"],
  [["b64enc", "foobar"], "Zm9vYmFy\n"],
  [["b64enc", "foob"], "Zm9vYg==\n"],
  [["b64enc", ""], "\n"],
  [["b64enc", "é"], "w6k=\n"],
  [["b64dec", "Zm9vYmFy"], "foobar"],
  [["b64dec", "w6k="], "é"],
  [["b64dec", ""], ""],
  [["roman", "1994"], "MCMXCIV\n"],
  [["roman", "3999"], "MMMCMXCIX\n"],
  [["roman", "1"], "I\n"],
  [["unroman", "MCMXCIV"], "1994\n"],
  [["unroman", "MMMCMXCIX"], "3999\n"],
  [["semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"], "-1\n"],
  [["semver-cmp", "1.0.0+a", "1.0.0+b"], "0\n"],
  [["semver-cmp", "2.0.0", "1.0.0"], "1\n"],
  [["semver-cmp", "1.0.0-rc.1", "1.0.0"], "-1\n"],
];

const ERROR_CASES: string[][] = [
  ["b64dec", "Zh=="],
  ["b64dec", "Zg="],
  ["b64dec", "AP_-"],
  ["b64dec", "Zg==Zg=="],
  ["b64dec", "Zm9v\n"],
  ["roman", "0"],
  ["roman", "4000"],
  ["roman", "-1"],
  ["roman", "abc"],
  ["roman", ""],
  ["roman", "1.5"],
  ["roman", "99999999999999999999999999"],
  ["unroman", "IIII"],
  ["unroman", "xiv"],
  ["unroman", ""],
  ["unroman", "VX"],
  ["semver-cmp", "01.0.0", "1.0.0"],
  ["semver-cmp", "1.0", "1.0.0"],
  ["semver-cmp", "1.0.0", "v1.0.0"],
];

const USAGE_CASES: string[][] = [
  [],
  ["bogus"],
  ["crc32"],
  ["crc32", "a", "b"],
  ["adler32"],
  ["b64enc"],
  ["b64dec"],
  ["roman"],
  ["unroman", "I", "II"],
  ["semver-cmp"],
  ["semver-cmp", "1.0.0"],
  ["semver-cmp", "1.0.0", "1.0.0", "1.0.0"],
  ["version", "extra"],
  ["--help"],
  ["toString"],
  ["constructor"],
  ["__proto__"],
];

function rows(cases: string[][]): [string[]][] {
  return cases.map((args) => [args]);
}

describe("cli", () => {
  test.each(OK_CASES)("ciutil %j succeeds", (args, expected) => {
    const result = runCli(...args);
    expect(result.code).toBe(0);
    expect(text(result.stdout)).toBe(expected);
    expect(result.stderr).toBe("");
  });

  test.each(rows(ERROR_CASES))("ciutil %j exits with 2", (args) => {
    const result = runCli(...args);
    expect(result.code).toBe(2);
    expect(result.stdout.length).toBe(0);
    expect(result.stderr.startsWith("ciutil: ")).toBe(true);
    expect(result.stderr.endsWith("\n")).toBe(true);
  });

  test.each(rows(USAGE_CASES))("ciutil %j exits with 64", (args) => {
    const result = runCli(...args);
    expect(result.code).toBe(64);
    expect(result.stdout.length).toBe(0);
    expect(result.stderr.startsWith("usage: ciutil")).toBe(true);
  });

  test("b64dec writes raw bytes without a trailing newline", () => {
    const result = runCli("b64dec", "AP/+");
    expect(Array.from(result.stdout)).toEqual([0x00, 0xff, 0xfe]);
  });

  test("b64dec of a lone 0xff byte is not re-encoded as text", () => {
    const result = runCli("b64dec", "/w==");
    expect(Array.from(result.stdout)).toEqual([0xff]);
  });

  test("crc32 hashes the UTF-8 bytes of the argument", () => {
    const utf8 = Uint8Array.of(0x68, 0xc3, 0xa9, 0x6c, 0x6c, 0x6f);
    const result = runCli("crc32", "héllo");
    expect(text(result.stdout)).toBe(`${hex32(crc32(utf8))}\n`);
  });

  test("output never contains a carriage return", () => {
    for (const args of OK_CASES.map(([input]) => input)) {
      expect(text(runCli(...args).stdout)).not.toContain("\r");
    }
  });
});
