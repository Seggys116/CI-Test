namespace CiUtil.Tests;

public class Base64Tests
{
    public static TheoryData<int> Lengths()
    {
        var data = new TheoryData<int>();
        for (int length = 0; length <= 300; length++)
        {
            data.Add(length);
        }

        return data;
    }

    [Theory]
    [InlineData("", "")]
    [InlineData("f", "Zg==")]
    [InlineData("fo", "Zm8=")]
    [InlineData("foo", "Zm9v")]
    [InlineData("foob", "Zm9vYg==")]
    [InlineData("fooba", "Zm9vYmE=")]
    [InlineData("foobar", "Zm9vYmFy")]
    public void EncodesKnownStrings(string input, string expected)
    {
        Assert.Equal(expected, Base64.Encode(TestData.Utf8(input)));
    }

    [Theory]
    [InlineData("", "")]
    [InlineData("f", "Zg==")]
    [InlineData("fo", "Zm8=")]
    [InlineData("foo", "Zm9v")]
    [InlineData("foob", "Zm9vYg==")]
    [InlineData("fooba", "Zm9vYmE=")]
    [InlineData("foobar", "Zm9vYmFy")]
    public void DecodesKnownStrings(string plain, string encoded)
    {
        Assert.Equal(TestData.Utf8(plain), Base64.Decode(encoded));
    }

    [Fact]
    public void EncodesNonTextBytes()
    {
        Assert.Equal("AP/+", Base64.Encode([0x00, 0xFF, 0xFE]));
    }

    [Fact]
    public void DecodesNonTextBytes()
    {
        Assert.Equal(new byte[] { 0x00, 0xFF, 0xFE }, Base64.Decode("AP/+"));
    }

    [Theory]
    [InlineData("Z")]
    [InlineData("Zg")]
    [InlineData("Zg=")]
    [InlineData("Zm9vY")]
    [InlineData("Zg==Zg==")]
    [InlineData("Zg=a")]
    [InlineData("=Zg=")]
    [InlineData("Z===")]
    [InlineData("====")]
    [InlineData("Zg===")]
    [InlineData("Zm9vYg===")]
    [InlineData("Zm9=Zm9v")]
    [InlineData("Zm=v")]
    [InlineData("Zm9v-A==")]
    [InlineData("Zm9v_A==")]
    [InlineData("ZmZ-")]
    [InlineData("Zm9_")]
    [InlineData("Zm9v\n")]
    [InlineData("Zm9v ")]
    [InlineData(" Zm9v")]
    [InlineData("Zm 9")]
    [InlineData("Zm9v\r\n")]
    [InlineData("Zmé")]
    [InlineData("Zm9é")]
    [InlineData("Zh==")]
    [InlineData("Zm9=")]
    public void RejectsInvalidInput(string text)
    {
        Assert.Throws<FormatException>(() => Base64.Decode(text));
    }

    [Theory]
    [MemberData(nameof(Lengths))]
    public void RoundTripsEveryLengthUpTo300(int length)
    {
        byte[] original = TestData.Lcg(300).AsSpan(0, length).ToArray();
        string encoded = Base64.Encode(original);
        Assert.Equal((length + 2) / 3 * 4, encoded.Length);
        Assert.Equal(original, Base64.Decode(encoded));
    }

    [Theory]
    [MemberData(nameof(Lengths))]
    public void EncodingMatchesTheFrameworkOracle(int length)
    {
        byte[] original = TestData.Lcg(300).AsSpan(0, length).ToArray();
        Assert.Equal(Convert.ToBase64String(original), Base64.Encode(original));
    }

    [Fact]
    public void RoundTripsEveryByteValue()
    {
        byte[] all = new byte[256];
        for (int i = 0; i < all.Length; i++)
        {
            all[i] = (byte)i;
        }

        Assert.Equal(all, Base64.Decode(Base64.Encode(all)));
    }
}
