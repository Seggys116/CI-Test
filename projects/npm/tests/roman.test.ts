import { describe, expect, test } from "vitest";
import { RomanError, fromRoman, toRoman } from "../src/roman.js";

const VECTORS: [number, string][] = [
  [1, "I"],
  [4, "IV"],
  [9, "IX"],
  [14, "XIV"],
  [40, "XL"],
  [90, "XC"],
  [400, "CD"],
  [1994, "MCMXCIV"],
  [2024, "MMXXIV"],
  [3999, "MMMCMXCIX"],
];

const INVALID_NUMERALS = [
  "",
  "IIII",
  "VX",
  "IC",
  "XM",
  "VV",
  "IIX",
  "xiv",
  "MMMM",
  "IXI",
  "VIV",
  "CMC",
  "IL",
  "XD",
  "XXXX",
  "LL",
  "DD",
  "IVI",
  " X",
  "X ",
  "X\n",
  "A",
  "Ⅻ",
];

const CANONICAL = /^M{0,3}(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})$/;

function allStrings(alphabet: string, maxLength: number): string[] {
  let level = [""];
  const all: string[] = [];
  for (let length = 1; length <= maxLength; length++) {
    level = level.flatMap((prefix) => [...alphabet].map((c) => prefix + c));
    all.push(...level);
  }
  return all;
}

describe("roman", () => {
  test.each(VECTORS)("toRoman(%i) is %s", (value, numeral) => {
    expect(toRoman(value)).toBe(numeral);
  });

  test.each(VECTORS)("fromRoman result for %i", (value, numeral) => {
    expect(fromRoman(numeral)).toBe(value);
  });

  test.each([0, -1, -3999, 4000, 4001, 1e6, 1.5, Number.NaN])(
    "toRoman rejects %s",
    (value) => {
      expect(() => toRoman(value)).toThrow(RomanError);
    },
  );

  test.each(INVALID_NUMERALS)("fromRoman rejects %j", (numeral) => {
    expect(() => fromRoman(numeral)).toThrow(RomanError);
  });

  test("fromRoman(toRoman(n)) equals n for every n in 1..3999", () => {
    const seen = new Set<string>();
    for (let value = 1; value <= 3999; value++) {
      const numeral = toRoman(value);
      expect(fromRoman(numeral)).toBe(value);
      expect(CANONICAL.test(numeral)).toBe(true);
      seen.add(numeral);
    }
    expect(seen.size).toBe(3999);
  });

  test("fromRoman accepts exactly the canonical forms up to length 4", () => {
    for (const candidate of allStrings("IVXLCDM", 4)) {
      let result: number | undefined;
      try {
        result = fromRoman(candidate);
      } catch (error) {
        expect(error).toBeInstanceOf(RomanError);
      }
      expect(result !== undefined).toBe(CANONICAL.test(candidate));
      if (result !== undefined) {
        expect(toRoman(result)).toBe(candidate);
      }
    }
  });
});
