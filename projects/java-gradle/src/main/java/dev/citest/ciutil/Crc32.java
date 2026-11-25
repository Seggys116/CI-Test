package dev.citest.ciutil;

import java.util.Objects;

/** CRC-32/IEEE (reflected, polynomial 0xEDB88320, init and xorout 0xFFFFFFFF). */
public final class Crc32 {

    private static final int[] TABLE = buildTable();

    private int state = 0xFFFFFFFF;

    private static int[] buildTable() {
        int[] table = new int[256];
        for (int index = 0; index < table.length; index++) {
            int crc = index;
            for (int bit = 0; bit < 8; bit++) {
                crc = (crc & 1) != 0 ? 0xEDB88320 ^ (crc >>> 1) : crc >>> 1;
            }
            table[index] = crc;
        }
        return table;
    }

    /** one-shot checksum, returned as an unsigned 32 bit value. */
    public static long checksum(byte[] data) {
        Crc32 crc = new Crc32();
        crc.update(data);
        return crc.value();
    }

    public void update(byte[] data) {
        update(data, 0, data.length);
    }

    public void update(byte[] data, int offset, int length) {
        Objects.checkFromIndexSize(offset, length, data.length);
        int crc = state;
        for (int i = offset; i < offset + length; i++) {
            crc = TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >>> 8);
        }
        state = crc;
    }

    /** checksum of everything passed to update so far; the instance stays usable. */
    public long value() {
        return (state ^ 0xFFFFFFFF) & 0xFFFFFFFFL;
    }
}
