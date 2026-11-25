namespace CiUtil;

/// <summary>CRC-32/IEEE: reflected, polynomial 0xEDB88320, init and xorout 0xFFFFFFFF.</summary>
public sealed class Crc32
{
    private static readonly uint[] Table = BuildTable();

    private uint _state = 0xFFFFFFFFu;

    /// <summary>Checksum of everything passed to <see cref="Update"/> so far; the instance stays usable.</summary>
    public uint Value => _state ^ 0xFFFFFFFFu;

    public static uint Compute(ReadOnlySpan<byte> data)
    {
        var crc = new Crc32();
        crc.Update(data);
        return crc.Value;
    }

    public void Update(ReadOnlySpan<byte> data)
    {
        uint crc = _state;
        foreach (byte b in data)
        {
            crc = Table[(crc ^ b) & 0xFF] ^ (crc >> 8);
        }

        _state = crc;
    }

    private static uint[] BuildTable()
    {
        var table = new uint[256];
        for (int index = 0; index < table.Length; index++)
        {
            uint crc = (uint)index;
            for (int bit = 0; bit < 8; bit++)
            {
                crc = (crc & 1) != 0 ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
            }

            table[index] = crc;
        }

        return table;
    }
}
