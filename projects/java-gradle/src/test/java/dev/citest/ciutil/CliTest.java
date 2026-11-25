package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.stream.Stream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.MethodSource;

class CliTest {

    private record Result(int code, byte[] out, byte[] err) {
        String outText() {
            return new String(out, StandardCharsets.UTF_8);
        }
    }

    private static Result invoke(String... args) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        ByteArrayOutputStream err = new ByteArrayOutputStream();
        PrintStream outStream = new PrintStream(out, false, StandardCharsets.UTF_8);
        PrintStream errStream = new PrintStream(err, false, StandardCharsets.UTF_8);
        int code = Cli.run(args, outStream, errStream);
        return new Result(code, out.toByteArray(), err.toByteArray());
    }

    @ParameterizedTest
    @CsvSource({
        "crc32, '', 00000000",
        "crc32, a, e8b7be43",
        "crc32, 123456789, cbf43926",
        "crc32, 'The quick brown fox jumps over the lazy dog', 414fa339",
        "adler32, '', 00000001",
        "adler32, Wikipedia, 11e60398",
        "adler32, 123456789, 091e01de",
        "b64enc, '', ''",
        "b64enc, foobar, Zm9vYmFy",
        "b64enc, fooba, Zm9vYmE=",
        "b64enc, f, Zg==",
        "roman, 1, I",
        "roman, 1994, MCMXCIV",
        "roman, 3999, MMMCMXCIX",
        "unroman, XIV, 14",
        "unroman, MMXXIV, 2024",
    })
    void lineOutputCommands(String command, String operand, String expected) {
        Result result = invoke(command, operand);
        assertEquals(0, result.code());
        assertEquals(expected + "\n", result.outText());
        assertEquals(0, result.err().length);
    }

    @ParameterizedTest
    @CsvSource({
        "1.0.0-alpha, 1.0.0-alpha.1, -1",
        "1.0.0+a, 1.0.0+b, 0",
        "2.1.1, 2.1.0, 1",
        "1.0.0-rc.1, 1.0.0, -1",
    })
    void semverCompare(String left, String right, String expected) {
        Result result = invoke("semver-cmp", left, right);
        assertEquals(0, result.code());
        assertEquals(expected + "\n", result.outText());
        assertEquals(0, result.err().length);
    }

    @Test
    void versionCommand() {
        Result result = invoke("version");
        assertEquals(0, result.code());
        assertEquals("ciutil java-gradle 1.0.0\n", result.outText());
    }

    @Test
    void decodeWritesRawBytesWithoutNewline() {
        Result result = invoke("b64dec", "AP/+");
        assertEquals(0, result.code());
        assertArrayEquals(new byte[] {0x00, (byte) 0xFF, (byte) 0xFE}, result.out());
    }

    @Test
    void decodeOfEmptyStringWritesNothing() {
        Result result = invoke("b64dec", "");
        assertEquals(0, result.code());
        assertEquals(0, result.out().length);
    }

    @Test
    void decodeOfTextWritesTheText() {
        assertEquals("foobar", invoke("b64dec", "Zm9vYmFy").outText());
    }

    static Stream<Arguments> errorCases() {
        return Stream.of(
                Arguments.of("roman", "0"),
                Arguments.of("roman", "4000"),
                Arguments.of("roman", "-1"),
                Arguments.of("roman", "abc"),
                Arguments.of("roman", ""),
                Arguments.of("roman", "+5"),
                Arguments.of("roman", "99999999999"),
                Arguments.of("unroman", "IIII"),
                Arguments.of("unroman", ""),
                Arguments.of("unroman", "xiv"),
                Arguments.of("b64dec", "Zg="),
                Arguments.of("b64dec", "Zh=="),
                Arguments.of("b64dec", "Zm9v\n"));
    }

    @ParameterizedTest
    @MethodSource("errorCases")
    void reportsErrorsWithExitCodeTwo(String command, String operand) {
        Result result = invoke(command, operand);
        assertEquals(2, result.code());
        assertEquals(0, result.out().length);
        assertTrue(result.err().length > 0);
    }

    @Test
    void invalidSemverReportsErrorWithExitCodeTwo() {
        Result result = invoke("semver-cmp", "1.0", "1.0.0");
        assertEquals(2, result.code());
        assertEquals(0, result.out().length);
        assertTrue(result.err().length > 0);
        assertEquals(2, invoke("semver-cmp", "1.0.0", "v1.0.0").code());
    }

    static Stream<Arguments> usageCases() {
        return Stream.of(
                Arguments.of((Object) new String[] {}),
                Arguments.of((Object) new String[] {""}),
                Arguments.of((Object) new String[] {"bogus"}),
                Arguments.of((Object) new String[] {"CRC32", "a"}),
                Arguments.of((Object) new String[] {"version", "extra"}),
                Arguments.of((Object) new String[] {"crc32"}),
                Arguments.of((Object) new String[] {"crc32", "a", "b"}),
                Arguments.of((Object) new String[] {"adler32"}),
                Arguments.of((Object) new String[] {"b64enc"}),
                Arguments.of((Object) new String[] {"b64enc", "a", "b"}),
                Arguments.of((Object) new String[] {"b64dec"}),
                Arguments.of((Object) new String[] {"roman"}),
                Arguments.of((Object) new String[] {"roman", "1", "2"}),
                Arguments.of((Object) new String[] {"unroman"}),
                Arguments.of((Object) new String[] {"semver-cmp"}),
                Arguments.of((Object) new String[] {"semver-cmp", "1.0.0"}),
                Arguments.of((Object) new String[] {"semver-cmp", "1.0.0", "1.0.0", "1.0.0"}));
    }

    @ParameterizedTest
    @MethodSource("usageCases")
    void reportsUsageWithExitCode64(String[] args) {
        Result result = invoke(args);
        assertEquals(64, result.code());
        assertEquals(0, result.out().length);
        assertTrue(result.err().length > 0);
    }

    @Test
    void nonAsciiArgumentsAreHashedAsUtf8() {
        // the two bytes c3 a9 are the utf-8 encoding of e with an acute accent
        String expected = TestData.hex8(Crc32.checksum(new byte[] {(byte) 0xC3, (byte) 0xA9}));
        assertEquals(expected + "\n", invoke("crc32", "é").outText());
        assertEquals("w6k=\n", invoke("b64enc", "é").outText());
    }

    @Test
    void outputUsesNewlinesOnly() {
        byte[] out = invoke("crc32", "abc").out();
        assertEquals((byte) 0x0A, out[out.length - 1]);
        for (byte b : out) {
            assertTrue(b != 0x0D);
        }
    }
}
