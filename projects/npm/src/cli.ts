import { adler32 } from "./adler32.js";
import { base64Decode, base64Encode } from "./base64.js";
import { crc32 } from "./crc32.js";
import { fromRoman, toRoman } from "./roman.js";
import { compareSemver, parseSemver } from "./semver.js";

export interface Io {
  stdout(data: Uint8Array): void;
  stderr(text: string): void;
}

interface Command {
  readonly arity: number;
  readonly run: (args: readonly string[]) => Uint8Array;
}

const USAGE = `usage: ciutil <command> [args]
commands:
  crc32 STRING
  adler32 STRING
  b64enc STRING
  b64dec BASE64
  roman NUMBER
  unroman ROMAN
  semver-cmp A B
  version
`;

const encoder = new TextEncoder();

function line(text: string): Uint8Array {
  return encoder.encode(`${text}\n`);
}

function hex32(value: number): string {
  return value.toString(16).padStart(8, "0");
}

function parseNumber(text: string): number {
  if (!/^-?[0-9]{1,15}$/.test(text)) {
    throw new Error(`invalid number: ${JSON.stringify(text)}`);
  }
  return Number(text);
}

function cmdCrc32(args: readonly string[]): Uint8Array {
  return line(hex32(crc32(encoder.encode(args[0]!))));
}

function cmdAdler32(args: readonly string[]): Uint8Array {
  return line(hex32(adler32(encoder.encode(args[0]!))));
}

function cmdB64Enc(args: readonly string[]): Uint8Array {
  return line(base64Encode(encoder.encode(args[0]!)));
}

function cmdB64Dec(args: readonly string[]): Uint8Array {
  return base64Decode(args[0]!);
}

function cmdRoman(args: readonly string[]): Uint8Array {
  return line(toRoman(parseNumber(args[0]!)));
}

function cmdUnroman(args: readonly string[]): Uint8Array {
  return line(String(fromRoman(args[0]!)));
}

function cmdSemverCmp(args: readonly string[]): Uint8Array {
  const a = parseSemver(args[0]!);
  const b = parseSemver(args[1]!);
  return line(String(compareSemver(a, b)));
}

function cmdVersion(): Uint8Array {
  return line("ciutil npm 1.0.0");
}

const COMMANDS = new Map<string, Command>([
  ["crc32", { arity: 1, run: cmdCrc32 }],
  ["adler32", { arity: 1, run: cmdAdler32 }],
  ["b64enc", { arity: 1, run: cmdB64Enc }],
  ["b64dec", { arity: 1, run: cmdB64Dec }],
  ["roman", { arity: 1, run: cmdRoman }],
  ["unroman", { arity: 1, run: cmdUnroman }],
  ["semver-cmp", { arity: 2, run: cmdSemverCmp }],
  ["version", { arity: 0, run: cmdVersion }],
]);

export function run(args: readonly string[], io: Io): number {
  const [name, ...params] = args;
  const command = name === undefined ? undefined : COMMANDS.get(name);
  if (command === undefined || params.length !== command.arity) {
    io.stderr(USAGE);
    return 64;
  }
  let output: Uint8Array;
  try {
    output = command.run(params);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    io.stderr(`ciutil: ${message}\n`);
    return 2;
  }
  io.stdout(output);
  return 0;
}
