import { assertEquals } from "@std/assert";
import { crc32 } from "../src/crc32.ts";
import { hex32, runCli, text } from "./helpers.ts";

const OK_CASES: [string[], string][] = [
  [["version"], "ciutil deno 1.0.0\n"],
  [["crc32", "123456789"], "cbf43926\n"],
  [["crc32", ""], "00000000\n"],
  [["crc32", "a"], "e8b7be43\n"],
  [["crc32", "The quick brown fox jumps over the lazy dog"], "414fa339\n"],
  [["adler32", "Wikipedia"], "11e60398\n"],
  [["adler32", ""], "00000001\n"],
  [["adler32", "123456789"], "091e01de\n"],
  [["b64enc", "foobar"], "Zm9vYmFy\n"],
  [["b64enc", "foob"], "Zm9vYg==\n"],
  [["b64enc", ""], "\n"],
  [["b64enc", "é"], "w6k=\n"],
  [["b64dec", "Zm9vYmFy"], "foobar"],
  [["b64dec", "w6k="], "é"],
  [["b64dec", ""], ""],
  [["roman", "1994"], "MCMXCIV\n"],
  [["roman", "3999"], "MMMCMXCIX\n"],
  [["roman", "1"], "I\n"],
  [["unroman", "MCMXCIV"], "1994\n"],
  [["unroman", "MMMCMXCIX"], "3999\n"],
  [["semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"], "-1\n"],
  [["semver-cmp", "1.0.0+a", "1.0.0+b"], "0\n"],
  [["semver-cmp", "2.0.0", "1.0.0"], "1\n"],
  [["semver-cmp", "1.0.0-rc.1", "1.0.0"], "-1\n"],
];

const ERROR_CASES: string[][] = [
  ["b64dec", "Zh=="],
  ["b64dec", "Zg="],
  ["b64dec", "AP_-"],
  ["b64dec", "Zg==Zg=="],
  ["b64dec", "Zm9v\n"],
  ["roman", "0"],
  ["roman", "4000"],
  ["roman", "-1"],
  ["roman", "abc"],
  ["roman", ""],
  ["roman", "1.5"],
  ["roman", "99999999999999999999999999"],
  ["unroman", "IIII"],
  ["unroman", "xiv"],
  ["unroman", ""],
  ["unroman", "VX"],
  ["semver-cmp", "01.0.0", "1.0.0"],
  ["semver-cmp", "1.0", "1.0.0"],
  ["semver-cmp", "1.0.0", "v1.0.0"],
];

const USAGE_CASES: string[][] = [
  [],
  ["bogus"],
  ["crc32"],
  ["crc32", "a", "b"],
  ["adler32"],
  ["b64enc"],
  ["b64dec"],
  ["roman"],
  ["unroman", "I", "II"],
  ["semver-cmp"],
  ["semver-cmp", "1.0.0"],
  ["semver-cmp", "1.0.0", "1.0.0", "1.0.0"],
  ["version", "extra"],
  ["--help"],
  ["toString"],
  ["constructor"],
  ["__proto__"],
];

interface SpawnResult {
  code: number;
  stdout: Uint8Array;
  stderr: string;
}

async function spawnCli(...args: string[]): Promise<SpawnResult> {
  const command = new Deno.Command(Deno.execPath(), {
    args: ["run", "src/main.ts", ...args],
    cwd: new URL("..", import.meta.url),
    stdout: "piped",
    stderr: "piped",
  });
  const output = await command.output();
  return {
    code: output.code,
    stdout: output.stdout,
    stderr: text(output.stderr),
  };
}

Deno.test("run() succeeds for valid invocations", async (t) => {
  for (const [args, expected] of OK_CASES) {
    await t.step(JSON.stringify(args), () => {
      const result = runCli(...args);
      assertEquals(result.code, 0);
      assertEquals(text(result.stdout), expected);
      assertEquals(result.stderr, "");
      assertEquals(text(result.stdout).includes("\r"), false);
    });
  }
});

Deno.test("run() exits with 2 on invalid values", async (t) => {
  for (const args of ERROR_CASES) {
    await t.step(JSON.stringify(args), () => {
      const result = runCli(...args);
      assertEquals(result.code, 2);
      assertEquals(result.stdout.length, 0);
      assertEquals(result.stderr.startsWith("ciutil: "), true);
      assertEquals(result.stderr.endsWith("\n"), true);
    });
  }
});

Deno.test("run() exits with 64 on bad usage", async (t) => {
  for (const args of USAGE_CASES) {
    await t.step(JSON.stringify(args), () => {
      const result = runCli(...args);
      assertEquals(result.code, 64);
      assertEquals(result.stdout.length, 0);
      assertEquals(result.stderr.startsWith("usage: ciutil"), true);
    });
  }
});

Deno.test("run() b64dec writes raw bytes without a newline", () => {
  assertEquals(Array.from(runCli("b64dec", "AP/+").stdout), [0, 255, 254]);
  assertEquals(Array.from(runCli("b64dec", "/w==").stdout), [255]);
});

Deno.test("run() crc32 hashes the UTF-8 bytes of the argument", () => {
  const utf8 = Uint8Array.of(0x68, 0xc3, 0xa9, 0x6c, 0x6c, 0x6f);
  const result = runCli("crc32", "héllo");
  assertEquals(text(result.stdout), `${hex32(crc32(utf8))}\n`);
});

Deno.test("deno run src/main.ts succeeds for valid invocations", async (t) => {
  for (const [args, expected] of OK_CASES) {
    await t.step(JSON.stringify(args), async () => {
      const result = await spawnCli(...args);
      assertEquals(result.code, 0);
      assertEquals(text(result.stdout), expected);
    });
  }
});

Deno.test("deno run src/main.ts exits with 2 on invalid values", async (t) => {
  for (const args of ERROR_CASES) {
    await t.step(JSON.stringify(args), async () => {
      const result = await spawnCli(...args);
      assertEquals(result.code, 2);
      assertEquals(result.stdout.length, 0);
      assertEquals(result.stderr.includes("ciutil: "), true);
    });
  }
});

Deno.test("deno run src/main.ts exits with 64 on bad usage", async (t) => {
  for (const args of USAGE_CASES) {
    await t.step(JSON.stringify(args), async () => {
      const result = await spawnCli(...args);
      assertEquals(result.code, 64);
      assertEquals(result.stdout.length, 0);
      assertEquals(result.stderr.includes("usage: ciutil"), true);
    });
  }
});

Deno.test("deno run src/main.ts emits raw bytes for b64dec", async () => {
  const result = await spawnCli("b64dec", "AP/+");
  assertEquals(Array.from(result.stdout), [0, 255, 254]);
});
