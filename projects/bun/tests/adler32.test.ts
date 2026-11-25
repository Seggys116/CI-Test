import { describe, expect, test } from "bun:test";
import {
  adler32,
  adler32Finalize,
  adler32Init,
  adler32Update,
} from "../src/adler32";
import { bytes, hex32, lcgBytes } from "./helpers";

const VECTORS: [string, string][] = [
  ["", "00000001"],
  ["a", "00620062"],
  ["abc", "024d0127"],
  ["Wikipedia", "11e60398"],
  ["123456789", "091e01de"],
];

function naiveAdler32(data: Uint8Array): number {
  let low = 1;
  let high = 0;
  for (const byte of data) {
    low = (low + byte) % 65521;
    high = (high + low) % 65521;
  }
  return ((high << 16) | low) >>> 0;
}

function chunkedAdler32(data: Uint8Array, size: number): number {
  let state = adler32Init();
  for (let start = 0; start < data.length; start += size) {
    state = adler32Update(state, data.subarray(start, start + size));
  }
  return adler32Finalize(state);
}

describe("adler32", () => {
  test.each(VECTORS)("text vector %j", (input, expected) => {
    expect(hex32(adler32(bytes(input)))).toBe(expected);
  });

  test("100000 bytes of 0xff match the naive reference", () => {
    const data = new Uint8Array(100000).fill(0xff);
    expect(adler32(data)).toBe(naiveAdler32(data));
  });

  test.each([1, 7, 5551, 5552, 5553, 65536])(
    "100000 bytes of 0xff in chunks of %i equal one-shot",
    (size) => {
      const data = new Uint8Array(100000).fill(0xff);
      expect(chunkedAdler32(data, size)).toBe(adler32(data));
    },
  );

  test.each([5551, 5552, 5553, 11104, 11105, 16657])(
    "length %i around the block size matches the naive reference",
    (length) => {
      const ones = new Uint8Array(length).fill(0xff);
      expect(adler32(ones)).toBe(naiveAdler32(ones));
      const random = lcgBytes(length);
      expect(adler32(random)).toBe(naiveAdler32(random));
    },
  );

  test("chunked equals one-shot at every split point", () => {
    const data = lcgBytes(1000);
    const expected = adler32(data);
    for (let split = 0; split <= data.length; split++) {
      let state = adler32Init();
      state = adler32Update(state, data.subarray(0, split));
      state = adler32Update(state, data.subarray(split));
      expect(adler32Finalize(state)).toBe(expected);
    }
  });

  test("result is an unsigned 32-bit integer", () => {
    const data = new Uint8Array(100000).fill(0xff);
    expect(adler32(data)).toBeGreaterThan(0);
    expect(adler32(data)).toBeLessThanOrEqual(0xffffffff);
    expect(adler32(bytes("Wikipedia"))).toBe(0x11e60398);
  });
});
