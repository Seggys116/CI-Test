const TABLE: readonly (readonly [number, string])[] = [
  [1000, "M"],
  [900, "CM"],
  [500, "D"],
  [400, "CD"],
  [100, "C"],
  [90, "XC"],
  [50, "L"],
  [40, "XL"],
  [10, "X"],
  [9, "IX"],
  [5, "V"],
  [4, "IV"],
  [1, "I"],
];

export class RomanError extends Error {}

export function toRoman(value: number): string {
  if (!Number.isInteger(value) || value < 1 || value > 3999) {
    throw new RomanError(`number out of range 1..3999: ${value}`);
  }
  let rest = value;
  let out = "";
  for (const [amount, symbol] of TABLE) {
    while (rest >= amount) {
      out += symbol;
      rest -= amount;
    }
  }
  return out;
}

export function fromRoman(text: string): number {
  let pos = 0;
  let total = 0;
  for (const [amount, symbol] of TABLE) {
    while (text.startsWith(symbol, pos)) {
      total += amount;
      pos += symbol.length;
    }
  }
  if (pos !== text.length || total === 0 || toRoman(total) !== text) {
    throw new RomanError(`invalid roman numeral: ${JSON.stringify(text)}`);
  }
  return total;
}
