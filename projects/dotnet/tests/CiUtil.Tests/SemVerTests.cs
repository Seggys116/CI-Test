namespace CiUtil.Tests;

public class SemVerTests
{
    private static readonly string[] Chain =
    [
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
    ];

    private static readonly string[] CoreChain = ["1.0.0", "2.0.0", "2.1.0", "2.1.1"];

    private static readonly string[] Accepted =
    [
        "0.0.0",
        "1.0.0",
        "1.0.0-0",
        "1.0.0-0a",
        "1.0.0-x-y-z.-",
        "1.0.0+001",
        "1.0.0-rc.1+build.5",
        "1.2.3-alpha.1.beta-2+exp.sha.5114f85",
        "10.20.30",
        "1.0.0-alpha-a.b-c-somethinglong+build.1-aef.1-its-okay",
        "18446744073709551615.18446744073709551615.18446744073709551615",
    ];

    [Theory]
    [InlineData("")]
    [InlineData("1")]
    [InlineData("1.0")]
    [InlineData("1.0.0.0")]
    [InlineData("v1.0.0")]
    [InlineData("01.0.0")]
    [InlineData("1.00.0")]
    [InlineData("1.0.00")]
    [InlineData("1.0.0-")]
    [InlineData("1.0.0+")]
    [InlineData("1.0.0-01")]
    [InlineData("1.0.0-a..b")]
    [InlineData("1.0.0-a_b")]
    [InlineData("-1.0.0")]
    [InlineData(" 1.0.0")]
    [InlineData("1.0.0 ")]
    [InlineData("1.0.0-alpha+")]
    [InlineData("1.0.0-alpha.")]
    [InlineData("1.0.0-.alpha")]
    [InlineData("1.0.0+a..b")]
    [InlineData("1.0.0+a_b")]
    [InlineData("1.0.0+a+b")]
    [InlineData("1.0.0\n")]
    [InlineData("1..0")]
    [InlineData("1.0.")]
    [InlineData(".1.0")]
    [InlineData("a.b.c")]
    [InlineData("1.0.x")]
    [InlineData("1.0.0-alpha.01")]
    [InlineData("+1.0.0")]
    [InlineData("1.0.0-é")]
    [InlineData("١.0.0")]
    [InlineData("18446744073709551616.0.0")]
    [InlineData("1.18446744073709551616.0")]
    [InlineData("1.0.99999999999999999999")]
    public void RejectsInvalidVersions(string text)
    {
        Assert.Throws<FormatException>(() => SemVer.Parse(text));
    }

    [Theory]
    [InlineData("0.0.0")]
    [InlineData("1.0.0")]
    [InlineData("1.0.0-0")]
    [InlineData("1.0.0-0a")]
    [InlineData("1.0.0-x-y-z.-")]
    [InlineData("1.0.0+001")]
    [InlineData("1.0.0-rc.1+build.5")]
    [InlineData("1.2.3-alpha.1.beta-2+exp.sha.5114f85")]
    [InlineData("10.20.30")]
    [InlineData("1.0.0-alpha-a.b-c-somethinglong+build.1-aef.1-its-okay")]
    [InlineData("18446744073709551615.18446744073709551615.18446744073709551615")]
    public void AcceptsValidVersionsAndRoundTripsThroughToString(string text)
    {
        SemVer version = SemVer.Parse(text);
        Assert.Equal(text, version.ToString());
        Assert.Equal(text, SemVer.Parse(version.ToString()).ToString());
    }

    [Fact]
    public void ParsesAllComponents()
    {
        SemVer version = SemVer.Parse("1.2.3-rc.1+build.5");
        Assert.Equal(1UL, version.Major);
        Assert.Equal(2UL, version.Minor);
        Assert.Equal(3UL, version.Patch);
        Assert.Equal(new[] { "rc", "1" }, version.Prerelease.ToArray());
        Assert.Equal(new[] { "build", "5" }, version.Build.ToArray());
    }

    [Fact]
    public void ParsesPlainVersion()
    {
        SemVer version = SemVer.Parse("0.0.0");
        Assert.Equal(0UL, version.Major);
        Assert.Empty(version.Prerelease);
        Assert.Empty(version.Build);
    }

    [Fact]
    public void ParsesLargestSixtyFourBitFields()
    {
        Assert.Equal(ulong.MaxValue, SemVer.Parse("18446744073709551615.0.0").Major);
    }

    [Fact]
    public void HyphensInsideIdentifiersAreKept()
    {
        Assert.Equal(new[] { "x-y-z", "-" }, SemVer.Parse("1.0.0-x-y-z.-").Prerelease.ToArray());
    }

    [Fact]
    public void OrderingChainHoldsForAllOrderedPairs()
    {
        SemVer[] versions = Chain.Select(SemVer.Parse).ToArray();
        for (int i = 0; i < versions.Length; i++)
        {
            Assert.Equal(0, SemVer.Compare(versions[i], versions[i]));
            for (int j = i + 1; j < versions.Length; j++)
            {
                Assert.True(SemVer.Compare(versions[i], versions[j]) == -1, $"{Chain[i]} < {Chain[j]}");
                Assert.True(SemVer.Compare(versions[j], versions[i]) == 1, $"{Chain[j]} > {Chain[i]}");
            }
        }
    }

    [Fact]
    public void CoreOrderingHoldsForAllOrderedPairs()
    {
        SemVer[] versions = CoreChain.Select(SemVer.Parse).ToArray();
        for (int i = 0; i < versions.Length; i++)
        {
            for (int j = i + 1; j < versions.Length; j++)
            {
                Assert.True(SemVer.Compare(versions[i], versions[j]) == -1, $"{CoreChain[i]} < {CoreChain[j]}");
                Assert.True(SemVer.Compare(versions[j], versions[i]) == 1, $"{CoreChain[j]} > {CoreChain[i]}");
            }
        }
    }

    [Fact]
    public void CompareIsAntisymmetricOverAllSamples()
    {
        string[] samples = [.. Chain, .. CoreChain, .. Accepted];
        foreach (string left in samples)
        {
            foreach (string right in samples)
            {
                int forward = SemVer.Compare(SemVer.Parse(left), SemVer.Parse(right));
                int backward = SemVer.Compare(SemVer.Parse(right), SemVer.Parse(left));
                Assert.True(forward == -backward, $"{left} vs {right}");
            }
        }
    }

    [Fact]
    public void BuildMetadataIsIgnored()
    {
        Assert.Equal(0, SemVer.Compare(SemVer.Parse("1.0.0+a"), SemVer.Parse("1.0.0+b")));
        Assert.Equal(0, SemVer.Compare(SemVer.Parse("1.0.0-rc.1+x"), SemVer.Parse("1.0.0-rc.1")));
    }

    [Fact]
    public void NumericIdentifiersCompareNumerically()
    {
        Assert.Equal(-1, SemVer.Compare(SemVer.Parse("1.0.0-beta.2"), SemVer.Parse("1.0.0-beta.11")));
    }

    [Fact]
    public void HugeNumericIdentifiersCompareWithoutOverflow()
    {
        SemVer smaller = SemVer.Parse("1.0.0-99999999999999999999999999");
        SemVer larger = SemVer.Parse("1.0.0-100000000000000000000000000");
        Assert.Equal(-1, SemVer.Compare(smaller, larger));
        Assert.Equal(1, SemVer.Compare(larger, smaller));
    }

    [Fact]
    public void NumericIdentifiersSortBeforeAlphanumeric()
    {
        SemVer numeric = SemVer.Parse("1.0.0-99999");
        SemVer alphanumeric = SemVer.Parse("1.0.0-a");
        Assert.Equal(-1, SemVer.Compare(numeric, alphanumeric));
        Assert.Equal(1, SemVer.Compare(alphanumeric, numeric));
    }

    [Fact]
    public void AlphanumericIdentifiersCompareInAsciiOrder()
    {
        Assert.Equal(-1, SemVer.Compare(SemVer.Parse("1.0.0-Z"), SemVer.Parse("1.0.0-a")));
        Assert.Equal(-1, SemVer.Compare(SemVer.Parse("1.0.0-a-"), SemVer.Parse("1.0.0-aa")));
    }

    [Fact]
    public void LongerPrereleaseListWinsWhenPrefixIsEqual()
    {
        Assert.Equal(-1, SemVer.Compare(SemVer.Parse("1.0.0-alpha"), SemVer.Parse("1.0.0-alpha.0")));
    }

    [Fact]
    public void ReleaseOutranksPrerelease()
    {
        Assert.Equal(1, SemVer.Compare(SemVer.Parse("2.0.0"), SemVer.Parse("2.0.0-rc.1")));
    }

    [Fact]
    public void PrereleaseOfALaterVersionOutranksEarlierRelease()
    {
        Assert.Equal(-1, SemVer.Compare(SemVer.Parse("1.9.9"), SemVer.Parse("2.0.0-alpha")));
    }
}
