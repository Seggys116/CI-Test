const UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const ALPHABET = UPPER + UPPER.toLowerCase() + "0123456789+/";

const DECODE = new Int16Array(256).fill(-1);
for (let i = 0; i < ALPHABET.length; i++) {
  DECODE[ALPHABET.charCodeAt(i)] = i;
}

// bits that must be zero in a group, indexed by its symbol count
const UNUSED_BITS = [0, 0, 0xffff, 0xff, 0];

export class Base64Error extends Error {}

function sextet(value: number): string {
  return ALPHABET.charAt(value & 63);
}

export function base64Encode(data: Uint8Array): string {
  let out = "";
  const full = data.length - (data.length % 3);
  for (let i = 0; i < full; i += 3) {
    const block = (data[i]! << 16) | (data[i + 1]! << 8) | data[i + 2]!;
    out += sextet(block >> 18) + sextet(block >> 12);
    out += sextet(block >> 6) + sextet(block);
  }
  const rest = data.length - full;
  if (rest === 1) {
    const block = data[full]! << 16;
    out += sextet(block >> 18) + sextet(block >> 12) + "==";
  } else if (rest === 2) {
    const block = (data[full]! << 16) | (data[full + 1]! << 8);
    out += sextet(block >> 18) + sextet(block >> 12);
    out += sextet(block >> 6) + "=";
  }
  return out;
}

function lookup(text: string, index: number): number {
  const value = DECODE[text.charCodeAt(index)];
  if (value === undefined || value < 0) {
    throw new Base64Error(`invalid character at position ${index}`);
  }
  return value;
}

export function base64Decode(text: string): Uint8Array {
  if (text.length % 4 !== 0) {
    throw new Base64Error("length is not a multiple of 4");
  }
  let pad = 0;
  if (text.endsWith("==")) {
    pad = 2;
  } else if (text.endsWith("=")) {
    pad = 1;
  }
  const bodyLength = text.length - pad;
  const out = new Uint8Array((text.length / 4) * 3 - pad);
  let pos = 0;
  for (let i = 0; i < bodyLength; i += 4) {
    const count = Math.min(4, bodyLength - i);
    let block = 0;
    for (let k = 0; k < 4; k++) {
      block = (block << 6) | (k < count ? lookup(text, i + k) : 0);
    }
    if ((block & UNUSED_BITS[count]!) !== 0) {
      throw new Base64Error("non-canonical trailing bits");
    }
    out[pos++] = (block >> 16) & 0xff;
    if (count > 2) {
      out[pos++] = (block >> 8) & 0xff;
    }
    if (count > 3) {
      out[pos++] = block & 0xff;
    }
  }
  return out;
}
