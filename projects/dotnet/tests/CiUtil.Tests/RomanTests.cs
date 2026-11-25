namespace CiUtil.Tests;

public class RomanTests
{
    [Theory]
    [InlineData(1, "I")]
    [InlineData(4, "IV")]
    [InlineData(9, "IX")]
    [InlineData(14, "XIV")]
    [InlineData(40, "XL")]
    [InlineData(90, "XC")]
    [InlineData(400, "CD")]
    [InlineData(1994, "MCMXCIV")]
    [InlineData(2024, "MMXXIV")]
    [InlineData(3999, "MMMCMXCIX")]
    public void ConvertsToRoman(int number, string numeral)
    {
        Assert.Equal(numeral, Roman.ToRoman(number));
    }

    [Theory]
    [InlineData(1, "I")]
    [InlineData(4, "IV")]
    [InlineData(9, "IX")]
    [InlineData(14, "XIV")]
    [InlineData(40, "XL")]
    [InlineData(90, "XC")]
    [InlineData(400, "CD")]
    [InlineData(1994, "MCMXCIV")]
    [InlineData(2024, "MMXXIV")]
    [InlineData(3999, "MMMCMXCIX")]
    public void ConvertsFromRoman(int number, string numeral)
    {
        Assert.Equal(number, Roman.FromRoman(numeral));
    }

    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    [InlineData(-3999)]
    [InlineData(4000)]
    [InlineData(4001)]
    [InlineData(int.MaxValue)]
    [InlineData(int.MinValue)]
    public void RejectsOutOfRangeNumbers(int number)
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => Roman.ToRoman(number));
    }

    [Theory]
    [InlineData("")]
    [InlineData("IIII")]
    [InlineData("VX")]
    [InlineData("IC")]
    [InlineData("XM")]
    [InlineData("VV")]
    [InlineData("IIX")]
    [InlineData("xiv")]
    [InlineData("iv")]
    [InlineData("IIV")]
    [InlineData("VIV")]
    [InlineData("XXXX")]
    [InlineData("LL")]
    [InlineData("DD")]
    [InlineData("CCCC")]
    [InlineData("MMMM")]
    [InlineData("MMMMCMXCIX")]
    [InlineData("IL")]
    [InlineData("XD")]
    [InlineData("VL")]
    [InlineData("CCM")]
    [InlineData("IXI")]
    [InlineData("IVI")]
    [InlineData("XIIII")]
    [InlineData("A")]
    [InlineData("I ")]
    [InlineData(" I")]
    [InlineData("I\n")]
    [InlineData("X-I")]
    [InlineData("0")]
    [InlineData("Ⅰ")]
    public void RejectsNonCanonicalNumerals(string text)
    {
        Assert.Throws<FormatException>(() => Roman.FromRoman(text));
    }

    [Fact]
    public void EveryNumberRoundTrips()
    {
        var failures = new List<int>();
        for (int number = 1; number <= 3999; number++)
        {
            if (Roman.FromRoman(Roman.ToRoman(number)) != number)
            {
                failures.Add(number);
            }
        }

        Assert.Empty(failures);
    }

    [Fact]
    public void NumeralsAreDistinctAndUppercase()
    {
        var seen = new HashSet<string>();
        for (int number = 1; number <= 3999; number++)
        {
            string numeral = Roman.ToRoman(number);
            Assert.True(numeral.All(c => "IVXLCDM".Contains(c, StringComparison.Ordinal)), numeral);
            seen.Add(numeral);
        }

        Assert.Equal(3999, seen.Count);
    }
}
