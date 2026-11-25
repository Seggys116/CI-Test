import { assertEquals } from "@std/assert";
import { crc32, crc32Finalize, crc32Init, crc32Update } from "../src/crc32.ts";
import { bytes, hex32, lcgBytes } from "./helpers.ts";

const VECTORS: [string, string][] = [
  ["", "00000000"],
  ["a", "e8b7be43"],
  ["abc", "352441c2"],
  ["123456789", "cbf43926"],
  ["The quick brown fox jumps over the lazy dog", "414fa339"],
];

Deno.test("crc32 text vectors", async (t) => {
  for (const [input, expected] of VECTORS) {
    await t.step(`vector ${JSON.stringify(input)}`, () => {
      assertEquals(hex32(crc32(bytes(input))), expected);
    });
  }
});

Deno.test("crc32 of 32 zero bytes", () => {
  assertEquals(hex32(crc32(new Uint8Array(32))), "190a55ad");
});

Deno.test("crc32 of 32 bytes of 0xff", () => {
  assertEquals(hex32(crc32(new Uint8Array(32).fill(0xff))), "ff6cab0b");
});

Deno.test("crc32 is an unsigned 32-bit integer", () => {
  assertEquals(crc32(bytes("abc")), 0x352441c2);
  assertEquals(crc32(new Uint8Array(32).fill(0xff)), 0xff6cab0b);
});

Deno.test("crc32 chunked equals one-shot at every split point", () => {
  const data = lcgBytes(1000);
  const expected = crc32(data);
  for (let split = 0; split <= data.length; split++) {
    let state = crc32Init();
    state = crc32Update(state, data.subarray(0, split));
    state = crc32Update(state, data.subarray(split));
    assertEquals(crc32Finalize(state), expected);
  }
});

Deno.test("crc32 byte at a time equals one-shot", () => {
  const data = lcgBytes(1000);
  let state = crc32Init();
  for (const byte of data) {
    state = crc32Update(state, Uint8Array.of(byte));
  }
  assertEquals(crc32Finalize(state), crc32(data));
});
