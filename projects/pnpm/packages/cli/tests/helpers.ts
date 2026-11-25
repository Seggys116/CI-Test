import { run } from "../src/cli.js";

const decoder = new TextDecoder();

export function text(data: Uint8Array): string {
  return decoder.decode(data);
}

export function hex32(value: number): string {
  return value.toString(16).padStart(8, "0");
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
  return { code, stdout: new Uint8Array(Buffer.concat(chunks)), stderr };
}
