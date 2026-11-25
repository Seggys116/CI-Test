import { crc32 as zlibCrc32 } from "node:zlib";
import { describe, expect, test } from "vitest";
import { crc32, crc32Finalize, crc32Init, crc32Update } from "../src/crc32.js";
import { bytes, hex32, lcgBytes } from "./helpers.js";

const VECTORS: [string, string][] = [
  ["", "00000000"],
  ["a", "e8b7be43"],
  ["abc", "352441c2"],
  ["123456789", "cbf43926"],
  ["The quick brown fox jumps over the lazy dog", "414fa339"],
];

describe("crc32", () => {
  test.each(VECTORS)("text vector %j", (input, expected) => {
    expect(hex32(crc32(bytes(input)))).toBe(expected);
  });

  test("32 zero bytes", () => {
    expect(hex32(crc32(new Uint8Array(32)))).toBe("190a55ad");
  });

  test("32 bytes of 0xff", () => {
    expect(hex32(crc32(new Uint8Array(32).fill(0xff)))).toBe("ff6cab0b");
  });

  test("result is an unsigned 32-bit integer", () => {
    expect(crc32(bytes("abc"))).toBe(0x352441c2);
    expect(crc32(new Uint8Array(32).fill(0xff))).toBe(0xff6cab0b);
  });

  test("chunked equals one-shot at every split point", () => {
    const data = lcgBytes(1000);
    const expected = crc32(data);
    for (let split = 0; split <= data.length; split++) {
      let state = crc32Init();
      state = crc32Update(state, data.subarray(0, split));
      state = crc32Update(state, data.subarray(split));
      expect(crc32Finalize(state)).toBe(expected);
    }
  });

  test("byte at a time equals one-shot", () => {
    const data = lcgBytes(1000);
    let state = crc32Init();
    for (const byte of data) {
      state = crc32Update(state, Uint8Array.of(byte));
    }
    expect(crc32Finalize(state)).toBe(crc32(data));
  });

  test("matches node zlib for every prefix of a pseudo-random input", () => {
    const data = lcgBytes(1000);
    for (let length = 0; length <= data.length; length++) {
      const prefix = data.subarray(0, length);
      expect(crc32(prefix)).toBe(zlibCrc32(prefix));
    }
  });
});
