package dev.citest.ciutil;

import java.io.PrintStream;
import java.nio.charset.StandardCharsets;

/** command line front end, kept free of System.* so tests can drive it directly. */
public final class Cli {

    public static final int EXIT_OK = 0;
    public static final int EXIT_ERROR = 2;
    public static final int EXIT_USAGE = 64;

    private static final String USAGE =
            """
            usage: ciutil <command> [arguments]
            commands:
              crc32 STRING
              adler32 STRING
              b64enc STRING
              b64dec BASE64
              roman NUMBER
              unroman ROMAN
              semver-cmp A B
              version
            """;

    private Cli() {}

    /**
     * runs one invocation. {@code args} excludes the program name; output is written as raw bytes.
     *
     * @return the process exit code: 0 on success, 2 on error, 64 on misuse
     */
    public static int run(String[] args, PrintStream out, PrintStream err) {
        int code;
        try {
            code = dispatch(args, out, err);
        } catch (IllegalArgumentException e) {
            writeText(err, "ciutil: " + e.getMessage() + "\n");
            code = EXIT_ERROR;
        }
        out.flush();
        err.flush();
        return code;
    }

    private static int dispatch(String[] args, PrintStream out, PrintStream err) {
        if (args.length == 0) {
            return usage(err);
        }
        String command = args[0];
        int operands = args.length - 1;
        switch (command) {
            case "version" -> {
                if (operands != 0) {
                    return usage(err);
                }
                writeText(out, "ciutil java-gradle 1.0.0\n");
            }
            case "crc32" -> {
                if (operands != 1) {
                    return usage(err);
                }
                writeText(out, hex32(Crc32.checksum(utf8(args[1]))) + "\n");
            }
            case "adler32" -> {
                if (operands != 1) {
                    return usage(err);
                }
                writeText(out, hex32(Adler32.checksum(utf8(args[1]))) + "\n");
            }
            case "b64enc" -> {
                if (operands != 1) {
                    return usage(err);
                }
                writeText(out, Base64.encode(utf8(args[1])) + "\n");
            }
            case "b64dec" -> {
                if (operands != 1) {
                    return usage(err);
                }
                byte[] decoded = Base64.decode(args[1]);
                out.write(decoded, 0, decoded.length);
            }
            case "roman" -> {
                if (operands != 1) {
                    return usage(err);
                }
                writeText(out, Roman.toRoman(parseDecimal(args[1])) + "\n");
            }
            case "unroman" -> {
                if (operands != 1) {
                    return usage(err);
                }
                writeText(out, Roman.fromRoman(args[1]) + "\n");
            }
            case "semver-cmp" -> {
                if (operands != 2) {
                    return usage(err);
                }
                int order = SemVer.compare(SemVer.parse(args[1]), SemVer.parse(args[2]));
                writeText(out, order + "\n");
            }
            default -> {
                return usage(err);
            }
        }
        return EXIT_OK;
    }

    private static int usage(PrintStream err) {
        writeText(err, USAGE);
        return EXIT_USAGE;
    }

    private static byte[] utf8(String text) {
        return text.getBytes(StandardCharsets.UTF_8);
    }

    private static void writeText(PrintStream stream, String text) {
        byte[] bytes = utf8(text);
        stream.write(bytes, 0, bytes.length);
    }

    private static String hex32(long value) {
        String digits = Long.toHexString(value);
        return "0".repeat(8 - digits.length()) + digits;
    }

    private static int parseDecimal(String text) {
        if (text.isEmpty()) {
            throw new IllegalArgumentException("not a non-negative decimal integer: " + text);
        }
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c < '0' || c > '9') {
                throw new IllegalArgumentException("not a non-negative decimal integer: " + text);
            }
        }
        try {
            return Integer.parseInt(text);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException("number too large: " + text, e);
        }
    }
}
