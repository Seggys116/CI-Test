namespace CiUtil;

/// <summary>Adler-32 as defined by RFC 1950.</summary>
public sealed class Adler32
{
    private const uint Modulus = 65521;

    // largest block length for which the sums cannot overflow 32 bits before reduction
    private const int BlockLimit = 5552;

    private uint _low = 1;
    private uint _high;

    /// <summary>Checksum of everything passed to <see cref="Update"/> so far; the instance stays usable.</summary>
    public uint Value => (_high << 16) | _low;

    public static uint Compute(ReadOnlySpan<byte> data)
    {
        var adler = new Adler32();
        adler.Update(data);
        return adler.Value;
    }

    public void Update(ReadOnlySpan<byte> data)
    {
        uint a = _low;
        uint b = _high;
        while (!data.IsEmpty)
        {
            int block = Math.Min(data.Length, BlockLimit);
            foreach (byte value in data[..block])
            {
                a += value;
                b += a;
            }

            a %= Modulus;
            b %= Modulus;
            data = data[block..];
        }

        _low = a;
        _high = b;
    }
}
