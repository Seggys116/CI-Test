package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.Arrays;
import java.util.stream.IntStream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.MethodSource;
import org.junit.jupiter.params.provider.ValueSource;

class Base64Test {

    @ParameterizedTest
    @CsvSource({
        "'', ''",
        "f, Zg==",
        "fo, Zm8=",
        "foo, Zm9v",
        "foob, Zm9vYg==",
        "fooba, Zm9vYmE=",
        "foobar, Zm9vYmFy",
    })
    void encodesKnownStrings(String input, String expected) {
        assertEquals(expected, Base64.encode(TestData.utf8(input)));
    }

    @ParameterizedTest
    @CsvSource({
        "'', ''",
        "f, Zg==",
        "fo, Zm8=",
        "foo, Zm9v",
        "foob, Zm9vYg==",
        "fooba, Zm9vYmE=",
        "foobar, Zm9vYmFy",
    })
    void decodesKnownStrings(String plain, String encoded) {
        assertArrayEquals(TestData.utf8(plain), Base64.decode(encoded));
    }

    @Test
    void encodesNonTextBytes() {
        assertEquals("AP/+", Base64.encode(new byte[] {0x00, (byte) 0xFF, (byte) 0xFE}));
    }

    @Test
    void decodesNonTextBytes() {
        assertArrayEquals(new byte[] {0x00, (byte) 0xFF, (byte) 0xFE}, Base64.decode("AP/+"));
    }

    @ParameterizedTest
    @ValueSource(
            strings = {
                "Z",
                "Zg",
                "Zg=",
                "Zm9vY",
                "Zg==Zg==",
                "Zg=a",
                "=Zg=",
                "Z===",
                "====",
                "Zg===",
                "Zm9vYg===",
                "Zm9=Zm9v",
                "Zm=v",
                "Zm9v-A==",
                "Zm9v_A==",
                "ZmZ-",
                "Zm9_",
                "Zm9v\n",
                "Zm9v ",
                " Zm9v",
                "Zm 9",
                "Zm9v\r\n",
                "Zmé",
                "Zm9é",
                "Zm😀",
                "Zh==",
                "Zm9=",
                "Zg=\u0000",
            })
    void rejectsInvalidInput(String text) {
        assertThrows(IllegalArgumentException.class, () -> Base64.decode(text));
    }

    static IntStream lengths() {
        return IntStream.rangeClosed(0, 300);
    }

    @ParameterizedTest
    @MethodSource("lengths")
    void roundTripsEveryLengthUpTo300(int length) {
        byte[] original = Arrays.copyOf(TestData.lcg(300), length);
        String encoded = Base64.encode(original);
        assertEquals((length + 2) / 3 * 4, encoded.length());
        assertArrayEquals(original, Base64.decode(encoded));
    }

    @Test
    void roundTripsEveryByteValue() {
        byte[] all = new byte[256];
        for (int i = 0; i < all.length; i++) {
            all[i] = (byte) i;
        }
        assertArrayEquals(all, Base64.decode(Base64.encode(all)));
    }
}
