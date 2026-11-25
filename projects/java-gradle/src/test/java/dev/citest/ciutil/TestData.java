package dev.citest.ciutil;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

final class TestData {

    private TestData() {}

    /** x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) and 0xFF, seed 42. */
    static byte[] lcg(int length) {
        byte[] bytes = new byte[length];
        long x = 42;
        for (int i = 0; i < length; i++) {
            x = (x * 1103515245L + 12345L) & 0x7FFFFFFFL;
            bytes[i] = (byte) (x >> 16);
        }
        return bytes;
    }

    static byte[] utf8(String text) {
        return text.getBytes(StandardCharsets.UTF_8);
    }

    static byte[] filled(int length, int value) {
        byte[] bytes = new byte[length];
        Arrays.fill(bytes, (byte) value);
        return bytes;
    }

    static String hex8(long value) {
        String digits = Long.toHexString(value);
        return "0".repeat(8 - digits.length()) + digits;
    }
}
