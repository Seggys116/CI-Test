package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.stream.IntStream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.MethodSource;
import org.junit.jupiter.params.provider.ValueSource;

class Adler32Test {

    /** reference with a modulo after every byte, so it never relies on the block limit. */
    private static long naiveAdler32(byte[] data) {
        long a = 1;
        long b = 0;
        for (byte value : data) {
            a = (a + (value & 0xFF)) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }

    @ParameterizedTest
    @CsvSource({
        "'', 00000001",
        "a, 00620062",
        "abc, 024d0127",
        "Wikipedia, 11e60398",
        "123456789, 091e01de",
    })
    void knownStrings(String input, String expected) {
        assertEquals(expected, TestData.hex8(Adler32.checksum(TestData.utf8(input))));
    }

    @Test
    void longInputOfFFBytesMatchesNaiveReference() {
        byte[] data = TestData.filled(100_000, 0xFF);
        assertEquals(naiveAdler32(data), Adler32.checksum(data));
    }

    @ParameterizedTest
    @ValueSource(ints = {5551, 5552, 5553, 11104, 11105, 100_000})
    void lengthsAroundTheBlockLimitMatchNaiveReference(int length) {
        byte[] data = TestData.filled(length, 0xFF);
        assertEquals(naiveAdler32(data), Adler32.checksum(data));
    }

    @Test
    void pseudoRandomInputMatchesNaiveReference() {
        byte[] data = TestData.lcg(20_000);
        assertEquals(naiveAdler32(data), Adler32.checksum(data));
    }

    @ParameterizedTest
    @ValueSource(ints = {1, 7, 5551, 5552, 5553, 65_536})
    void chunkedLongInputMatchesOneShot(int chunkSize) {
        byte[] data = TestData.filled(100_000, 0xFF);
        Adler32 adler = new Adler32();
        for (int start = 0; start < data.length; start += chunkSize) {
            adler.update(data, start, Math.min(chunkSize, data.length - start));
        }
        assertEquals(Adler32.checksum(data), adler.value());
    }

    static IntStream splitPoints() {
        return IntStream.rangeClosed(0, 1000);
    }

    @ParameterizedTest
    @MethodSource("splitPoints")
    void chunkedMatchesOneShotAtEverySplit(int split) {
        byte[] data = TestData.lcg(1000);
        Adler32 adler = new Adler32();
        adler.update(data, 0, split);
        adler.update(data, split, data.length - split);
        assertEquals(Adler32.checksum(data), adler.value());
    }
}
