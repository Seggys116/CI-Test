package dev.citest.ciutil;

import java.util.Objects;

/** Adler-32 as defined by RFC 1950. */
public final class Adler32 {

    private static final long MODULUS = 65521;
    // largest block length for which the sums cannot overflow 32 unsigned bits before reduction
    private static final int BLOCK_LIMIT = 5552;

    private long low = 1;
    private long high = 0;

    /** one-shot checksum, returned as an unsigned 32 bit value. */
    public static long checksum(byte[] data) {
        Adler32 adler = new Adler32();
        adler.update(data);
        return adler.value();
    }

    public void update(byte[] data) {
        update(data, 0, data.length);
    }

    public void update(byte[] data, int offset, int length) {
        Objects.checkFromIndexSize(offset, length, data.length);
        long a = low;
        long b = high;
        int position = offset;
        int remaining = length;
        while (remaining > 0) {
            int block = Math.min(remaining, BLOCK_LIMIT);
            for (int end = position + block; position < end; position++) {
                a += data[position] & 0xFF;
                b += a;
            }
            a %= MODULUS;
            b %= MODULUS;
            remaining -= block;
        }
        low = a;
        high = b;
    }

    /** checksum of everything passed to update so far; the instance stays usable. */
    public long value() {
        return (high << 16) | low;
    }
}
