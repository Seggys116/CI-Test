import { run } from "../src/cli.ts";

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

export function concat(chunks: Uint8Array[]): Uint8Array {
  const total = chunks.reduce((sum, chunk) => sum + chunk.length, 0);
  const out = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) {
    out.set(chunk, offset);
    offset += chunk.length;
  }
  return out;
}

export interface CliResult {
  code: number;
  stdout: Uint8Array;
  stderr: string;
}

export function runCli(...args: string[]): CliResult {
  const chunks: Uint8Array[] = [];
  let stderr = "";
  const code = run(args, {
    stdout: (data) => {
      chunks.push(data);
    },
    stderr: (message) => {
      stderr += message;
    },
  });
  return { code, stdout: concat(chunks), stderr };
}
