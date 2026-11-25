#!/usr/bin/env node
import { run } from "./cli.js";

process.exitCode = run(process.argv.slice(2), {
  stdout: (data) => process.stdout.write(data),
  stderr: (text) => process.stderr.write(text),
});
