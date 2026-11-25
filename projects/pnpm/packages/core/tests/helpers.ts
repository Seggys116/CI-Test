const encoder = new TextEncoder();
const decoder = new TextDecoder();

export function bytes(text: string): Uint8Array {
  return encoder.encode(text);
}

export function text(data: Uint8Array): string {
  return decoder.decode(data);
}

export function hex32(value: number): string {
  return value.toString(16).padStart(8, "0");
}

export function lcgBytes(count: number, seed = 42): Uint8Array {
  let state = seed;
  const out = new Uint8Array(count);
  for (let i = 0; i < count; i++) {
    state = (Math.imul(state, 1103515245) + 12345) & 0x7fffffff;
    out[i] = (state >>> 16) & 0xff;
  }
  return out;
}
