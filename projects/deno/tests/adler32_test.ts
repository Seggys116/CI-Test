import { assert, assertEquals } from "@std/assert";
import {
  adler32,
  adler32Finalize,
  adler32Init,
  adler32Update,
} from "../src/adler32.ts";
import { bytes, hex32, lcgBytes } from "./helpers.ts";

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

Deno.test("adler32 text vectors", async (t) => {
  for (const [input, expected] of VECTORS) {
    await t.step(`vector ${JSON.stringify(input)}`, () => {
      assertEquals(hex32(adler32(bytes(input))), expected);
    });
  }
});

Deno.test("adler32 of 100000 0xff bytes matches the naive one", () => {
  const data = new Uint8Array(100000).fill(0xff);
  assertEquals(adler32(data), naiveAdler32(data));
});

Deno.test("adler32 chunked equals one-shot on long input", async (t) => {
  const data = new Uint8Array(100000).fill(0xff);
  for (const size of [1, 7, 5551, 5552, 5553, 65536]) {
    await t.step(`chunks of ${size}`, () => {
      assertEquals(chunkedAdler32(data, size), adler32(data));
    });
  }
});

Deno.test("adler32 near the block size matches naive", async (t) => {
  for (const length of [5551, 5552, 5553, 11104, 11105, 16657]) {
    await t.step(`length ${length}`, () => {
      const ones = new Uint8Array(length).fill(0xff);
      assertEquals(adler32(ones), naiveAdler32(ones));
      const random = lcgBytes(length);
      assertEquals(adler32(random), naiveAdler32(random));
    });
  }
});

Deno.test("adler32 chunked equals one-shot at every split point", () => {
  const data = lcgBytes(1000);
  const expected = adler32(data);
  for (let split = 0; split <= data.length; split++) {
    let state = adler32Init();
    state = adler32Update(state, data.subarray(0, split));
    state = adler32Update(state, data.subarray(split));
    assertEquals(adler32Finalize(state), expected);
  }
});

Deno.test("adler32 is an unsigned 32-bit integer", () => {
  const data = new Uint8Array(100000).fill(0xff);
  assert(adler32(data) > 0);
  assert(adler32(data) <= 0xffffffff);
  assertEquals(adler32(bytes("Wikipedia")), 0x11e60398);
});
