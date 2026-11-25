import { assertEquals, assertThrows } from "@std/assert";
import { base64Decode, base64Encode, Base64Error } from "../src/base64.ts";
import { bytes, lcgBytes, text } from "./helpers.ts";

const VECTORS: [string, string][] = [
  ["", ""],
  ["f", "Zg=="],
  ["fo", "Zm8="],
  ["foo", "Zm9v"],
  ["foob", "Zm9vYg=="],
  ["fooba", "Zm9vYmE="],
  ["foobar", "Zm9vYmFy"],
];

const REJECTED = [
  "A",
  "Zg",
  "Zg=",
  "AAA",
  "Zm9vY",
  "Zm9v\n",
  "Zm-v",
  "Zm_v",
  "AP_-",
  "-_-_",
  "Zm9\n",
  "Zm9\r",
  "Zm 9",
  " Zm9",
  "Zm9 ",
  "Zm9\t",
  "Zm9é",
  "éééé",
  "Zg==Zg==",
  "Zg==Zm9v",
  "Zg=A",
  "=Zg=",
  "Z=g=",
  "A=AA",
  "=AAA",
  "Z===",
  "====",
  "Zm9v====",
  "Zg===",
  "Zh==",
  "Zm9=",
  "ZH==",
  "AAB=",
];

Deno.test("base64 encodes the RFC 4648 vectors", async (t) => {
  for (const [input, expected] of VECTORS) {
    await t.step(`encodes ${JSON.stringify(input)}`, () => {
      assertEquals(base64Encode(bytes(input)), expected);
    });
  }
});

Deno.test("base64 decodes the RFC 4648 vectors", async (t) => {
  for (const [expected, encoded] of VECTORS) {
    await t.step(`decodes ${JSON.stringify(encoded)}`, () => {
      assertEquals(text(base64Decode(encoded)), expected);
    });
  }
});

Deno.test("base64 handles the bytes 00 ff fe", () => {
  assertEquals(base64Encode(Uint8Array.of(0x00, 0xff, 0xfe)), "AP/+");
  assertEquals(Array.from(base64Decode("AP/+")), [0x00, 0xff, 0xfe]);
});

Deno.test("base64 rejects malformed input", async (t) => {
  for (const input of REJECTED) {
    await t.step(`rejects ${JSON.stringify(input)}`, () => {
      assertThrows(() => base64Decode(input), Base64Error);
    });
  }
});

Deno.test("base64 round trips every length from 0 to 300", () => {
  const source = lcgBytes(300);
  for (let length = 0; length <= 300; length++) {
    const data = source.subarray(0, length);
    assertEquals(base64Decode(base64Encode(data)), data);
  }
});

Deno.test("base64 matches the built-in btoa for every length", () => {
  const source = lcgBytes(300);
  for (let length = 0; length <= 300; length++) {
    const data = source.subarray(0, length);
    const binary = Array.from(data, (byte) => String.fromCharCode(byte));
    assertEquals(base64Encode(data), btoa(binary.join("")));
  }
});

Deno.test("base64 round trips every first byte with edge second bytes", () => {
  for (let first = 0; first < 256; first++) {
    for (const second of [0, 1, 127, 128, 255]) {
      const data = Uint8Array.of(first, second);
      assertEquals(base64Decode(base64Encode(data)), data);
    }
  }
});

Deno.test("base64 encoding any decodable text reproduces the text", () => {
  const alphabet = "AQgw=";
  for (const a of alphabet) {
    for (const b of alphabet) {
      for (const c of alphabet) {
        for (const d of alphabet) {
          const candidate = a + b + c + d;
          let decoded: Uint8Array | undefined;
          try {
            decoded = base64Decode(candidate);
          } catch (error) {
            assertEquals(error instanceof Base64Error, true);
          }
          if (decoded !== undefined) {
            assertEquals(base64Encode(decoded), candidate);
          }
        }
      }
    }
  }
});
