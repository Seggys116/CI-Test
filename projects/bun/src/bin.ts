#!/usr/bin/env bun
import { run } from "./cli";

process.exitCode = run(process.argv.slice(2), {
  stdout: (data) => process.stdout.write(data),
  stderr: (text) => process.stderr.write(text),
});
