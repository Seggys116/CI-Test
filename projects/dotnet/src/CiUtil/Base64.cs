namespace CiUtil;

/// <summary>RFC 4648 standard alphabet with padding; the decoder is strict.</summary>
public static class Base64
{
    private const string Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    private const char Padding = '=';

    // -1 marks characters outside the alphabet
    private static readonly int[] Reverse = BuildReverse();

    public static string Encode(ReadOnlySpan<byte> data)
    {
        var output = new char[(data.Length + 2) / 3 * 4];
        int written = 0;
        int index = 0;
        while (index + 3 <= data.Length)
        {
            int group = (data[index] << 16) | (data[index + 1] << 8) | data[index + 2];
            output[written++] = Alphabet[(group >> 18) & 63];
            output[written++] = Alphabet[(group >> 12) & 63];
            output[written++] = Alphabet[(group >> 6) & 63];
            output[written++] = Alphabet[group & 63];
            index += 3;
        }

        int rest = data.Length - index;
        if (rest == 1)
        {
            int group = data[index] << 16;
            output[written++] = Alphabet[(group >> 18) & 63];
            output[written++] = Alphabet[(group >> 12) & 63];
            output[written++] = Padding;
            output[written] = Padding;
        }
        else if (rest == 2)
        {
            int group = (data[index] << 16) | (data[index + 1] << 8);
            output[written++] = Alphabet[(group >> 18) & 63];
            output[written++] = Alphabet[(group >> 12) & 63];
            output[written++] = Alphabet[(group >> 6) & 63];
            output[written] = Padding;
        }

        return new string(output);
    }

    /// <exception cref="FormatException">The text is not canonical padded base64.</exception>
    public static byte[] Decode(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        if (text.Length % 4 != 0)
        {
            throw new FormatException("invalid base64: length is not a multiple of 4");
        }

        int blockCount = text.Length / 4;
        var result = new List<byte>(blockCount * 3);
        for (int block = 0; block < blockCount; block++)
        {
            uint group = 0;
            int padCount = 0;
            for (int offset = 0; offset < 4; offset++)
            {
                char c = text[(block * 4) + offset];
                int sextet = 0;
                if (c == Padding)
                {
                    // padding is only legal as the last one or two characters of the final block
                    if (block != blockCount - 1 || offset < 2)
                    {
                        throw new FormatException("invalid base64: misplaced padding");
                    }

                    padCount++;
                }
                else
                {
                    if (padCount != 0)
                    {
                        throw new FormatException("invalid base64: data after padding");
                    }

                    sextet = c < Reverse.Length ? Reverse[c] : -1;
                    if (sextet < 0)
                    {
                        throw new FormatException("invalid base64: character not in the alphabet");
                    }
                }

                group = (group << 6) | (uint)sextet;
            }

            result.Add((byte)(group >> 16));
            if (padCount == 0)
            {
                result.Add((byte)(group >> 8));
                result.Add((byte)group);
            }
            else if (padCount == 1)
            {
                if ((group & 0xFF) != 0)
                {
                    throw new FormatException("invalid base64: non-canonical trailing bits");
                }

                result.Add((byte)(group >> 8));
            }
            else if ((group & 0xFFFF) != 0)
            {
                throw new FormatException("invalid base64: non-canonical trailing bits");
            }
        }

        return result.ToArray();
    }

    private static int[] BuildReverse()
    {
        var table = new int[128];
        Array.Fill(table, -1);
        for (int i = 0; i < Alphabet.Length; i++)
        {
            table[Alphabet[i]] = i;
        }

        return table;
    }
}
