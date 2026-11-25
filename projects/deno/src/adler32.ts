const MOD = 65521;
const NMAX = 5552;

export function adler32Init(): number {
  return 1;
}

export function adler32Update(state: number, data: Uint8Array): number {
  let low = state & 0xffff;
  let high = state >>> 16;
  for (let start = 0; start < data.length; start += NMAX) {
    const end = Math.min(start + NMAX, data.length);
    for (let i = start; i < end; i++) {
      low += data[i]!;
      high += low;
    }
    low %= MOD;
    high %= MOD;
  }
  return ((high << 16) | low) >>> 0;
}

export function adler32Finalize(state: number): number {
  return state;
}

export function adler32(data: Uint8Array): number {
  return adler32Finalize(adler32Update(adler32Init(), data));
}
