using System.Text;

namespace CiUtil.Tests;

public class CliTests
{
    private sealed record Outcome(int Code, byte[] Out, byte[] Err)
    {
        public string OutText => Encoding.UTF8.GetString(Out);
    }

    private static Outcome Invoke(params string[] args)
    {
        using var stdout = new MemoryStream();
        using var stderr = new MemoryStream();
        int code = Cli.Run(args, stdout, stderr);
        return new Outcome(code, stdout.ToArray(), stderr.ToArray());
    }

    [Theory]
    [InlineData("crc32", "", "00000000")]
    [InlineData("crc32", "a", "e8b7be43")]
    [InlineData("crc32", "123456789", "cbf43926")]
    [InlineData("crc32", "The quick brown fox jumps over the lazy dog", "414fa339")]
    [InlineData("adler32", "", "00000001")]
    [InlineData("adler32", "Wikipedia", "11e60398")]
    [InlineData("adler32", "123456789", "091e01de")]
    [InlineData("b64enc", "", "")]
    [InlineData("b64enc", "foobar", "Zm9vYmFy")]
    [InlineData("b64enc", "fooba", "Zm9vYmE=")]
    [InlineData("b64enc", "f", "Zg==")]
    [InlineData("roman", "1", "I")]
    [InlineData("roman", "1994", "MCMXCIV")]
    [InlineData("roman", "3999", "MMMCMXCIX")]
    [InlineData("unroman", "XIV", "14")]
    [InlineData("unroman", "MMXXIV", "2024")]
    public void LineOutputCommands(string command, string operand, string expected)
    {
        Outcome result = Invoke(command, operand);
        Assert.Equal(0, result.Code);
        Assert.Equal(expected + "\n", result.OutText);
        Assert.Empty(result.Err);
    }

    [Theory]
    [InlineData("1.0.0-alpha", "1.0.0-alpha.1", "-1")]
    [InlineData("1.0.0+a", "1.0.0+b", "0")]
    [InlineData("2.1.1", "2.1.0", "1")]
    [InlineData("1.0.0-rc.1", "1.0.0", "-1")]
    public void SemverCompare(string left, string right, string expected)
    {
        Outcome result = Invoke("semver-cmp", left, right);
        Assert.Equal(0, result.Code);
        Assert.Equal(expected + "\n", result.OutText);
        Assert.Empty(result.Err);
    }

    [Fact]
    public void VersionCommand()
    {
        Outcome result = Invoke("version");
        Assert.Equal(0, result.Code);
        Assert.Equal("ciutil dotnet 1.0.0\n", result.OutText);
    }

    [Fact]
    public void DecodeWritesRawBytesWithoutNewline()
    {
        Outcome result = Invoke("b64dec", "AP/+");
        Assert.Equal(0, result.Code);
        Assert.Equal(new byte[] { 0x00, 0xFF, 0xFE }, result.Out);
    }

    [Fact]
    public void DecodeOfEmptyStringWritesNothing()
    {
        Outcome result = Invoke("b64dec", "");
        Assert.Equal(0, result.Code);
        Assert.Empty(result.Out);
    }

    [Fact]
    public void DecodeOfTextWritesTheText()
    {
        Assert.Equal("foobar", Invoke("b64dec", "Zm9vYmFy").OutText);
    }

    [Theory]
    [InlineData("roman", "0")]
    [InlineData("roman", "4000")]
    [InlineData("roman", "-1")]
    [InlineData("roman", "abc")]
    [InlineData("roman", "")]
    [InlineData("roman", "+5")]
    [InlineData("roman", "99999999999")]
    [InlineData("unroman", "IIII")]
    [InlineData("unroman", "")]
    [InlineData("unroman", "xiv")]
    [InlineData("b64dec", "Zg=")]
    [InlineData("b64dec", "Zh==")]
    [InlineData("b64dec", "Zm9v\n")]
    public void ReportsErrorsWithExitCodeTwo(string command, string operand)
    {
        Outcome result = Invoke(command, operand);
        Assert.Equal(2, result.Code);
        Assert.Empty(result.Out);
        Assert.NotEmpty(result.Err);
    }

    [Theory]
    [InlineData("1.0", "1.0.0")]
    [InlineData("1.0.0", "v1.0.0")]
    public void InvalidSemverReportsErrorWithExitCodeTwo(string left, string right)
    {
        Outcome result = Invoke("semver-cmp", left, right);
        Assert.Equal(2, result.Code);
        Assert.Empty(result.Out);
        Assert.NotEmpty(result.Err);
    }

    // null entries mean the argument is absent
    [Theory]
    [InlineData(null, null, null, null)]
    [InlineData("", null, null, null)]
    [InlineData("bogus", null, null, null)]
    [InlineData("CRC32", "a", null, null)]
    [InlineData("version", "extra", null, null)]
    [InlineData("crc32", null, null, null)]
    [InlineData("crc32", "a", "b", null)]
    [InlineData("adler32", null, null, null)]
    [InlineData("b64enc", null, null, null)]
    [InlineData("b64enc", "a", "b", null)]
    [InlineData("b64dec", null, null, null)]
    [InlineData("roman", null, null, null)]
    [InlineData("roman", "1", "2", null)]
    [InlineData("unroman", null, null, null)]
    [InlineData("semver-cmp", null, null, null)]
    [InlineData("semver-cmp", "1.0.0", null, null)]
    [InlineData("semver-cmp", "1.0.0", "1.0.0", "1.0.0")]
    public void ReportsUsageWithExitCode64(string? first, string? second, string? third, string? fourth)
    {
        string[] args = new[] { first, second, third, fourth }.OfType<string>().ToArray();
        Outcome result = Invoke(args);
        Assert.Equal(64, result.Code);
        Assert.Empty(result.Out);
        Assert.NotEmpty(result.Err);
    }

    [Fact]
    public void NonAsciiArgumentsAreHashedAsUtf8()
    {
        // the two bytes c3 a9 are the utf-8 encoding of e with an acute accent
        string expected = TestData.Hex8(Crc32.Compute([0xC3, 0xA9]));
        Assert.Equal(expected + "\n", Invoke("crc32", "é").OutText);
        Assert.Equal("w6k=\n", Invoke("b64enc", "é").OutText);
    }

    [Fact]
    public void OutputUsesNewlinesOnlyAndNoByteOrderMark()
    {
        byte[] output = Invoke("crc32", "abc").Out;
        Assert.Equal((byte)0x0A, output[^1]);
        Assert.DoesNotContain((byte)0x0D, output);
        Assert.NotEqual((byte)0xEF, output[0]);
    }
}
