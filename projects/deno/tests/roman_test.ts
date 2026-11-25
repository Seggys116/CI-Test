import { assertEquals, assertThrows } from "@std/assert";
import { fromRoman, RomanError, toRoman } from "../src/roman.ts";

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

Deno.test("toRoman vectors", async (t) => {
  for (const [value, numeral] of VECTORS) {
    await t.step(`toRoman(${value})`, () => {
      assertEquals(toRoman(value), numeral);
    });
  }
});

Deno.test("fromRoman vectors", async (t) => {
  for (const [value, numeral] of VECTORS) {
    await t.step(`fromRoman(${numeral})`, () => {
      assertEquals(fromRoman(numeral), value);
    });
  }
});

Deno.test("toRoman rejects out of range values", async (t) => {
  for (const value of [0, -1, -3999, 4000, 4001, 1e6, 1.5, Number.NaN]) {
    await t.step(`toRoman(${value})`, () => {
      assertThrows(() => toRoman(value), RomanError);
    });
  }
});

Deno.test("fromRoman rejects non-canonical input", async (t) => {
  for (const numeral of INVALID_NUMERALS) {
    await t.step(`fromRoman(${JSON.stringify(numeral)})`, () => {
      assertThrows(() => fromRoman(numeral), RomanError);
    });
  }
});

Deno.test("fromRoman(toRoman(n)) equals n for every n in 1..3999", () => {
  const seen = new Set<string>();
  for (let value = 1; value <= 3999; value++) {
    const numeral = toRoman(value);
    assertEquals(fromRoman(numeral), value);
    assertEquals(CANONICAL.test(numeral), true);
    seen.add(numeral);
  }
  assertEquals(seen.size, 3999);
});

Deno.test("fromRoman accepts exactly the canonical forms up to 4", () => {
  for (const candidate of allStrings("IVXLCDM", 4)) {
    let result: number | undefined;
    try {
      result = fromRoman(candidate);
    } catch (error) {
      assertEquals(error instanceof RomanError, true);
    }
    assertEquals(result !== undefined, CANONICAL.test(candidate));
    if (result !== undefined) {
      assertEquals(toRoman(result), candidate);
    }
  }
});
