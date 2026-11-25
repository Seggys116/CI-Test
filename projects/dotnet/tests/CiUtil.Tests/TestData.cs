using System.Globalization;
using System.Text;

namespace CiUtil.Tests;

internal static class TestData
{
    /// <summary>x = x * 1103515245 + 12345 mod 2^31, byte = (x >> 16) and 0xFF, seed 42.</summary>
    public static byte[] Lcg(int length)
    {
        var bytes = new byte[length];
        ulong x = 42;
        for (int i = 0; i < length; i++)
        {
            x = ((x * 1103515245UL) + 12345UL) & 0x7FFFFFFFUL;
            bytes[i] = (byte)(x >> 16);
        }

        return bytes;
    }

    public static byte[] Utf8(string text) => Encoding.UTF8.GetBytes(text);

    public static byte[] Filled(int length, byte value)
    {
        var bytes = new byte[length];
        Array.Fill(bytes, value);
        return bytes;
    }

    public static string Hex8(uint value) => value.ToString("x8", CultureInfo.InvariantCulture);
}
