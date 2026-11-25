import { describe, expect, test } from "vitest";
import { Base64Error, base64Decode, base64Encode } from "../src/base64.js";
import { bytes, lcgBytes, text } from "./helpers.js";

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

describe("base64", () => {
  test.each(VECTORS)("encodes %j", (input, expected) => {
    expect(base64Encode(bytes(input))).toBe(expected);
  });

  test.each(VECTORS)("decodes to %j", (expected, encoded) => {
    expect(text(base64Decode(encoded))).toBe(expected);
  });

  test("encodes the bytes 00 ff fe as AP/+", () => {
    expect(base64Encode(Uint8Array.of(0x00, 0xff, 0xfe))).toBe("AP/+");
  });

  test("decodes AP/+ to the bytes 00 ff fe", () => {
    expect(Array.from(base64Decode("AP/+"))).toEqual([0x00, 0xff, 0xfe]);
  });

  test.each(REJECTED)("rejects %j", (input) => {
    expect(() => base64Decode(input)).toThrow(Base64Error);
  });

  test("round trips every length from 0 to 300", () => {
    const source = lcgBytes(300);
    for (let length = 0; length <= 300; length++) {
      const data = source.subarray(0, length);
      expect(base64Decode(base64Encode(data))).toEqual(data);
    }
  });

  test("matches node Buffer encoding for every length from 0 to 300", () => {
    const source = lcgBytes(300);
    for (let length = 0; length <= 300; length++) {
      const data = source.subarray(0, length);
      expect(base64Encode(data)).toBe(Buffer.from(data).toString("base64"));
    }
  });

  test("round trips every first byte with edge second bytes", () => {
    for (let first = 0; first < 256; first++) {
      for (const second of [0, 1, 127, 128, 255]) {
        const data = Uint8Array.of(first, second);
        expect(base64Decode(base64Encode(data))).toEqual(data);
      }
    }
  });

  test("encoding any decodable text reproduces the text", () => {
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
              expect(error).toBeInstanceOf(Base64Error);
            }
            if (decoded !== undefined) {
              expect(base64Encode(decoded)).toBe(candidate);
            }
          }
        }
      }
    }
  });
});
