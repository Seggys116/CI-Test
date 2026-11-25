const POLY = 0xedb88320;
const MASK = 0xffffffff;

function makeTable(): Uint32Array {
  const table = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let value = i;
    for (let bit = 0; bit < 8; bit++) {
      value = (value & 1) !== 0 ? (value >>> 1) ^ POLY : value >>> 1;
    }
    table[i] = value;
  }
  return table;
}

const TABLE = makeTable();

export function crc32Init(): number {
  return MASK;
}

export function crc32Update(state: number, data: Uint8Array): number {
  let value = state;
  for (const byte of data) {
    value = TABLE[(value ^ byte) & 0xff]! ^ (value >>> 8);
  }
  return value >>> 0;
}

export function crc32Finalize(state: number): number {
  return (state ^ MASK) >>> 0;
}

export function crc32(data: Uint8Array): number {
  return crc32Finalize(crc32Update(crc32Init(), data));
}
