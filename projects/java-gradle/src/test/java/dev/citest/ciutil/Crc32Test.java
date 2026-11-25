package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.stream.IntStream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.MethodSource;

class Crc32Test {

    @ParameterizedTest
    @CsvSource({
        "'', 00000000",
        "a, e8b7be43",
        "abc, 352441c2",
        "123456789, cbf43926",
        "'The quick brown fox jumps over the lazy dog', 414fa339",
    })
    void knownStrings(String input, String expected) {
        assertEquals(expected, TestData.hex8(Crc32.checksum(TestData.utf8(input))));
    }

    @Test
    void thirtyTwoZeroBytes() {
        assertEquals("190a55ad", TestData.hex8(Crc32.checksum(TestData.filled(32, 0x00))));
    }

    @Test
    void thirtyTwoFFBytes() {
        assertEquals("ff6cab0b", TestData.hex8(Crc32.checksum(TestData.filled(32, 0xFF))));
    }

    static IntStream splitPoints() {
        return IntStream.rangeClosed(0, 1000);
    }

    @ParameterizedTest
    @MethodSource("splitPoints")
    void chunkedMatchesOneShotAtEverySplit(int split) {
        byte[] data = TestData.lcg(1000);
        Crc32 crc = new Crc32();
        crc.update(data, 0, split);
        crc.update(data, split, data.length - split);
        assertEquals(Crc32.checksum(data), crc.value());
    }

    @Test
    void singleByteUpdatesMatchOneShot() {
        byte[] data = TestData.lcg(1000);
        Crc32 crc = new Crc32();
        for (byte b : data) {
            crc.update(new byte[] {b});
        }
        assertEquals(Crc32.checksum(data), crc.value());
    }

    @Test
    void valueDoesNotConsumeState() {
        Crc32 crc = new Crc32();
        crc.update(TestData.utf8("1234"));
        long intermediate = crc.value();
        assertEquals(intermediate, crc.value());
        crc.update(TestData.utf8("56789"));
        assertEquals(0xcbf43926L, crc.value());
    }

    @Test
    void rejectsOutOfBoundsRange() {
        Crc32 crc = new Crc32();
        byte[] data = new byte[4];
        assertThrows(IndexOutOfBoundsException.class, () -> crc.update(data, 2, 3));
        assertThrows(IndexOutOfBoundsException.class, () -> crc.update(data, -1, 1));
    }
}
