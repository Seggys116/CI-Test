import { fileURLToPath } from "node:url";
import { defineConfig } from "vitest/config";

const coreSource = fileURLToPath(
  new URL("../core/src/index.ts", import.meta.url),
);

export default defineConfig({
  resolve: {
    alias: {
      "@ciutil/core": coreSource,
    },
  },
  test: {
    name: "cli",
  },
});
