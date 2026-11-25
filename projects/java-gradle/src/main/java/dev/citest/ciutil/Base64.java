package dev.citest.ciutil;

import java.io.ByteArrayOutputStream;
import java.util.Arrays;

/** RFC 4648 standard alphabet with padding; the decoder is strict. */
public final class Base64 {

    private static final String ALPHABET =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    private static final char PADDING = '=';

    // -1 marks characters outside the alphabet
    private static final int[] REVERSE = buildReverse();

    private Base64() {}

    private static int[] buildReverse() {
        int[] table = new int[128];
        Arrays.fill(table, -1);
        for (int i = 0; i < ALPHABET.length(); i++) {
            table[ALPHABET.charAt(i)] = i;
        }
        return table;
    }

    public static String encode(byte[] data) {
        StringBuilder out = new StringBuilder((data.length + 2) / 3 * 4);
        int index = 0;
        while (index + 3 <= data.length) {
            int group =
                    ((data[index] & 0xFF) << 16)
                            | ((data[index + 1] & 0xFF) << 8)
                            | (data[index + 2] & 0xFF);
            out.append(ALPHABET.charAt((group >>> 18) & 63));
            out.append(ALPHABET.charAt((group >>> 12) & 63));
            out.append(ALPHABET.charAt((group >>> 6) & 63));
            out.append(ALPHABET.charAt(group & 63));
            index += 3;
        }
        int rest = data.length - index;
        if (rest == 1) {
            int group = (data[index] & 0xFF) << 16;
            out.append(ALPHABET.charAt((group >>> 18) & 63));
            out.append(ALPHABET.charAt((group >>> 12) & 63));
            out.append(PADDING).append(PADDING);
        } else if (rest == 2) {
            int group = ((data[index] & 0xFF) << 16) | ((data[index + 1] & 0xFF) << 8);
            out.append(ALPHABET.charAt((group >>> 18) & 63));
            out.append(ALPHABET.charAt((group >>> 12) & 63));
            out.append(ALPHABET.charAt((group >>> 6) & 63));
            out.append(PADDING);
        }
        return out.toString();
    }

    /**
     * decodes strictly.
     *
     * @throws IllegalArgumentException for any input that is not canonical padded base64
     */
    public static byte[] decode(String text) {
        if (text.length() % 4 != 0) {
            throw new IllegalArgumentException("invalid base64: length is not a multiple of 4");
        }
        int blockCount = text.length() / 4;
        ByteArrayOutputStream out = new ByteArrayOutputStream(blockCount * 3);

        for (int block = 0; block < blockCount; block++) {
            int group = 0;
            int padCount = 0;
            for (int offset = 0; offset < 4; offset++) {
                char c = text.charAt(block * 4 + offset);
                int sextet = 0;
                if (c == PADDING) {
                    // padding is only legal as the last one or two characters of the final block
                    if (block != blockCount - 1 || offset < 2) {
                        throw new IllegalArgumentException("invalid base64: misplaced padding");
                    }
                    padCount++;
                } else {
                    if (padCount != 0) {
                        throw new IllegalArgumentException("invalid base64: data after padding");
                    }
                    sextet = c < REVERSE.length ? REVERSE[c] : -1;
                    if (sextet < 0) {
                        throw new IllegalArgumentException(
                                "invalid base64: character not in the alphabet at index "
                                        + (block * 4 + offset));
                    }
                }
                group = (group << 6) | sextet;
            }

            out.write(group >>> 16);
            if (padCount == 0) {
                out.write(group >>> 8);
                out.write(group);
            } else if (padCount == 1) {
                if ((group & 0xFF) != 0) {
                    throw new IllegalArgumentException("invalid base64: non-canonical trailing bits");
                }
                out.write(group >>> 8);
            } else if ((group & 0xFFFF) != 0) {
                throw new IllegalArgumentException("invalid base64: non-canonical trailing bits");
            }
        }
        return out.toByteArray();
    }
}
