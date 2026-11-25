import { type Io, run } from "./cli.ts";

interface Sink {
  writeSync(data: Uint8Array): number;
}

const encoder = new TextEncoder();

function writeAll(sink: Sink, data: Uint8Array): void {
  let offset = 0;
  while (offset < data.length) {
    offset += sink.writeSync(data.subarray(offset));
  }
}

const io: Io = {
  stdout: (data) => writeAll(Deno.stdout, data),
  stderr: (text) => writeAll(Deno.stderr, encoder.encode(text)),
};

if (import.meta.main) {
  Deno.exit(run(Deno.args, io));
}
