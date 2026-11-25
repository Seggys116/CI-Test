package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.HashSet;
import java.util.Set;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.ValueSource;

class RomanTest {

    @ParameterizedTest
    @CsvSource({
        "1, I",
        "4, IV",
        "9, IX",
        "14, XIV",
        "40, XL",
        "90, XC",
        "400, CD",
        "1994, MCMXCIV",
        "2024, MMXXIV",
        "3999, MMMCMXCIX",
    })
    void convertsToRoman(int number, String numeral) {
        assertEquals(numeral, Roman.toRoman(number));
    }

    @ParameterizedTest
    @CsvSource({
        "1, I",
        "4, IV",
        "9, IX",
        "14, XIV",
        "40, XL",
        "90, XC",
        "400, CD",
        "1994, MCMXCIV",
        "2024, MMXXIV",
        "3999, MMMCMXCIX",
    })
    void convertsFromRoman(int number, String numeral) {
        assertEquals(number, Roman.fromRoman(numeral));
    }

    @ParameterizedTest
    @ValueSource(ints = {0, -1, -3999, 4000, 4001, Integer.MAX_VALUE, Integer.MIN_VALUE})
    void rejectsOutOfRangeNumbers(int number) {
        assertThrows(IllegalArgumentException.class, () -> Roman.toRoman(number));
    }

    @ParameterizedTest
    @ValueSource(
            strings = {
                "",
                "IIII",
                "VX",
                "IC",
                "XM",
                "VV",
                "IIX",
                "xiv",
                "iv",
                "IIV",
                "VIV",
                "XXXX",
                "LL",
                "DD",
                "CCCC",
                "MMMM",
                "MMMMCMXCIX",
                "IL",
                "XD",
                "VL",
                "CCM",
                "IXI",
                "IVI",
                "XIIII",
                "A",
                "I ",
                " I",
                "I\n",
                "X-I",
                "0",
                "Ⅰ",
            })
    void rejectsNonCanonicalNumerals(String text) {
        assertThrows(IllegalArgumentException.class, () -> Roman.fromRoman(text));
    }

    @Test
    void everyNumberRoundTrips() {
        for (int number = 1; number <= 3999; number++) {
            String numeral = Roman.toRoman(number);
            assertEquals(number, Roman.fromRoman(numeral), "round trip of " + number);
        }
    }

    @Test
    void numeralsAreDistinctAndUppercase() {
        Set<String> seen = new HashSet<>();
        for (int number = 1; number <= 3999; number++) {
            String numeral = Roman.toRoman(number);
            assertTrue(numeral.matches("[IVXLCDM]+"), numeral);
            seen.add(numeral);
        }
        assertEquals(3999, seen.size());
    }
}
