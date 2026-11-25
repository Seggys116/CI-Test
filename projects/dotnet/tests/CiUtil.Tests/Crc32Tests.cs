namespace CiUtil.Tests;

public class Crc32Tests
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

    [Theory]
    [InlineData("", "00000000")]
    [InlineData("a", "e8b7be43")]
    [InlineData("abc", "352441c2")]
    [InlineData("123456789", "cbf43926")]
    [InlineData("The quick brown fox jumps over the lazy dog", "414fa339")]
    public void KnownStrings(string input, string expected)
    {
        Assert.Equal(expected, TestData.Hex8(Crc32.Compute(TestData.Utf8(input))));
    }

    [Fact]
    public void ThirtyTwoZeroBytes()
    {
        Assert.Equal("190a55ad", TestData.Hex8(Crc32.Compute(TestData.Filled(32, 0x00))));
    }

    [Fact]
    public void ThirtyTwoFFBytes()
    {
        Assert.Equal("ff6cab0b", TestData.Hex8(Crc32.Compute(TestData.Filled(32, 0xFF))));
    }

    [Theory]
    [MemberData(nameof(SplitPoints))]
    public void ChunkedMatchesOneShotAtEverySplit(int split)
    {
        byte[] data = TestData.Lcg(1000);
        var crc = new Crc32();
        crc.Update(data.AsSpan(0, split));
        crc.Update(data.AsSpan(split));
        Assert.Equal(Crc32.Compute(data), crc.Value);
    }

    [Fact]
    public void SingleByteUpdatesMatchOneShot()
    {
        byte[] data = TestData.Lcg(1000);
        var crc = new Crc32();
        foreach (byte b in data)
        {
            crc.Update([b]);
        }

        Assert.Equal(Crc32.Compute(data), crc.Value);
    }

    [Fact]
    public void ValueDoesNotConsumeState()
    {
        var crc = new Crc32();
        crc.Update(TestData.Utf8("1234"));
        uint intermediate = crc.Value;
        Assert.Equal(intermediate, crc.Value);
        crc.Update(TestData.Utf8("56789"));
        Assert.Equal(0xcbf43926u, crc.Value);
    }
}
