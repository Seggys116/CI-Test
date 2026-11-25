namespace CiUtil.Tests;

public class Adler32Tests
{
    public static TheoryData<int> SplitPoints()
    {
        var data = new TheoryData<int>();
        for (int split = 0; split <= 1000; split++)
        {
            data.Add(split);
        }

        return data;
    }

    // reference with a modulo after every byte, so it never relies on the block limit
    private static uint NaiveAdler32(byte[] data)
    {
        ulong a = 1;
        ulong b = 0;
        foreach (byte value in data)
        {
            a = (a + value) % 65521;
            b = (b + a) % 65521;
        }

        return (uint)((b << 16) | a);
    }

    [Theory]
    [InlineData("", "00000001")]
    [InlineData("a", "00620062")]
    [InlineData("abc", "024d0127")]
    [InlineData("Wikipedia", "11e60398")]
    [InlineData("123456789", "091e01de")]
    public void KnownStrings(string input, string expected)
    {
        Assert.Equal(expected, TestData.Hex8(Adler32.Compute(TestData.Utf8(input))));
    }

    [Fact]
    public void LongInputOfFFBytesMatchesNaiveReference()
    {
        byte[] data = TestData.Filled(100_000, 0xFF);
        Assert.Equal(NaiveAdler32(data), Adler32.Compute(data));
    }

    [Theory]
    [InlineData(5551)]
    [InlineData(5552)]
    [InlineData(5553)]
    [InlineData(11104)]
    [InlineData(11105)]
    [InlineData(100_000)]
    public void LengthsAroundTheBlockLimitMatchNaiveReference(int length)
    {
        byte[] data = TestData.Filled(length, 0xFF);
        Assert.Equal(NaiveAdler32(data), Adler32.Compute(data));
    }

    [Fact]
    public void PseudoRandomInputMatchesNaiveReference()
    {
        byte[] data = TestData.Lcg(20_000);
        Assert.Equal(NaiveAdler32(data), Adler32.Compute(data));
    }

    [Theory]
    [InlineData(1)]
    [InlineData(7)]
    [InlineData(5551)]
    [InlineData(5552)]
    [InlineData(5553)]
    [InlineData(65_536)]
    public void ChunkedLongInputMatchesOneShot(int chunkSize)
    {
        byte[] data = TestData.Filled(100_000, 0xFF);
        var adler = new Adler32();
        for (int start = 0; start < data.Length; start += chunkSize)
        {
            adler.Update(data.AsSpan(start, Math.Min(chunkSize, data.Length - start)));
        }

        Assert.Equal(Adler32.Compute(data), adler.Value);
    }

    [Theory]
    [MemberData(nameof(SplitPoints))]
    public void ChunkedMatchesOneShotAtEverySplit(int split)
    {
        byte[] data = TestData.Lcg(1000);
        var adler = new Adler32();
        adler.Update(data.AsSpan(0, split));
        adler.Update(data.AsSpan(split));
        Assert.Equal(Adler32.Compute(data), adler.Value);
    }
}
