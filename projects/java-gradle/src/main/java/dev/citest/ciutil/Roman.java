package dev.citest.ciutil;

/** roman numerals for 1 to 3999; parsing accepts only the canonical spelling. */
public final class Roman {

    private static final int[] VALUES = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    private static final String[] SYMBOLS = {
        "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"
    };

    private Roman() {}

    /**
     * @throws IllegalArgumentException if the number is outside 1 to 3999
     */
    public static String toRoman(int number) {
        if (number < 1 || number > 3999) {
            throw new IllegalArgumentException(number + " is outside the range 1 to 3999");
        }
        int remaining = number;
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < VALUES.length; i++) {
            while (remaining >= VALUES[i]) {
                out.append(SYMBOLS[i]);
                remaining -= VALUES[i];
            }
        }
        return out.toString();
    }

    /**
     * @throws IllegalArgumentException if the text is not a canonical roman numeral
     */
    public static int fromRoman(String text) {
        if (text.isEmpty()) {
            throw new IllegalArgumentException("empty roman numeral");
        }
        int[] digits = new int[text.length()];
        for (int i = 0; i < digits.length; i++) {
            digits[i] = symbolValue(text.charAt(i));
            if (digits[i] == 0) {
                throw new IllegalArgumentException("invalid roman numeral: " + text);
            }
        }

        int total = 0;
        for (int i = 0; i < digits.length; i++) {
            if (i + 1 < digits.length && digits[i] < digits[i + 1]) {
                total -= digits[i];
            } else {
                total += digits[i];
            }
        }

        // the canonical spelling of a number is unique, so re-encoding rejects every other form
        if (total < 1 || total > 3999 || !toRoman(total).equals(text)) {
            throw new IllegalArgumentException("invalid roman numeral: " + text);
        }
        return total;
    }

    private static int symbolValue(char symbol) {
        return switch (symbol) {
            case 'I' -> 1;
            case 'V' -> 5;
            case 'X' -> 10;
            case 'L' -> 50;
            case 'C' -> 100;
            case 'D' -> 500;
            case 'M' -> 1000;
            default -> 0;
        };
    }
}
