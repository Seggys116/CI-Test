using System.Globalization;

namespace CiUtil;

/// <summary>A Semantic Versioning 2.0.0 version. Comparison follows precedence and ignores build metadata.</summary>
public sealed class SemVer
{
    public SemVer(
        ulong major,
        ulong minor,
        ulong patch,
        IReadOnlyList<string>? prerelease = null,
        IReadOnlyList<string>? build = null)
    {
        Major = major;
        Minor = minor;
        Patch = patch;
        Prerelease = prerelease?.ToArray() ?? [];
        Build = build?.ToArray() ?? [];
    }

    public ulong Major { get; }

    public ulong Minor { get; }

    public ulong Patch { get; }

    public IReadOnlyList<string> Prerelease { get; }

    public IReadOnlyList<string> Build { get; }

    /// <exception cref="FormatException">The text is not a valid semantic version.</exception>
    public static SemVer Parse(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        string core = text;

        string[] build = [];
        int plus = core.IndexOf('+', StringComparison.Ordinal);
        if (plus >= 0)
        {
            build = ParseIdentifiers(core[(plus + 1)..], allowLeadingZeros: true);
            core = core[..plus];
        }

        string[] prerelease = [];
        int dash = core.IndexOf('-', StringComparison.Ordinal);
        if (dash >= 0)
        {
            prerelease = ParseIdentifiers(core[(dash + 1)..], allowLeadingZeros: false);
            core = core[..dash];
        }

        string[] fields = core.Split('.');
        if (fields.Length != 3)
        {
            throw Invalid(text, "expected MAJOR.MINOR.PATCH");
        }

        return new SemVer(
            ParseNumber(fields[0], text),
            ParseNumber(fields[1], text),
            ParseNumber(fields[2], text),
            prerelease,
            build);
    }

    /// <summary>Returns -1, 0 or 1 by semver precedence; build metadata is ignored.</summary>
    public static int Compare(SemVer left, SemVer right)
    {
        ArgumentNullException.ThrowIfNull(left);
        ArgumentNullException.ThrowIfNull(right);

        int result = left.Major.CompareTo(right.Major);
        if (result == 0)
        {
            result = left.Minor.CompareTo(right.Minor);
        }

        if (result == 0)
        {
            result = left.Patch.CompareTo(right.Patch);
        }

        if (result != 0)
        {
            return Math.Sign(result);
        }

        if (left.Prerelease.Count == 0 || right.Prerelease.Count == 0)
        {
            if (left.Prerelease.Count == 0 && right.Prerelease.Count == 0)
            {
                return 0;
            }

            // a release outranks any of its prereleases
            return left.Prerelease.Count == 0 ? 1 : -1;
        }

        int shared = Math.Min(left.Prerelease.Count, right.Prerelease.Count);
        for (int i = 0; i < shared; i++)
        {
            int identifier = CompareIdentifiers(left.Prerelease[i], right.Prerelease[i]);
            if (identifier != 0)
            {
                return identifier;
            }
        }

        return Math.Sign(left.Prerelease.Count.CompareTo(right.Prerelease.Count));
    }

    public override string ToString()
    {
        string text = string.Create(CultureInfo.InvariantCulture, $"{Major}.{Minor}.{Patch}");
        if (Prerelease.Count > 0)
        {
            text += "-" + string.Join('.', Prerelease);
        }

        if (Build.Count > 0)
        {
            text += "+" + string.Join('.', Build);
        }

        return text;
    }

    private static FormatException Invalid(string text, string reason) =>
        new($"invalid semantic version \"{text}\": {reason}");

    private static ulong ParseNumber(string field, string text)
    {
        if (!IsAllDigits(field))
        {
            throw Invalid(text, "version fields must be non-negative integers");
        }

        if (field.Length > 1 && field[0] == '0')
        {
            throw Invalid(text, "version fields must not have leading zeros");
        }

        ulong value = 0;
        foreach (char c in field)
        {
            ulong digit = (ulong)(c - '0');
            if (value > (ulong.MaxValue - digit) / 10)
            {
                throw Invalid(text, "version field does not fit in 64 bits");
            }

            value = (value * 10) + digit;
        }

        return value;
    }

    private static string[] ParseIdentifiers(string part, bool allowLeadingZeros)
    {
        string[] identifiers = part.Split('.');
        foreach (string identifier in identifiers)
        {
            if (identifier.Length == 0)
            {
                throw new FormatException("invalid semantic version: empty identifier");
            }

            foreach (char c in identifier)
            {
                if (!IsIdentifierChar(c))
                {
                    throw new FormatException("invalid semantic version: identifiers may only contain [0-9A-Za-z-]");
                }
            }

            if (!allowLeadingZeros && identifier.Length > 1 && identifier[0] == '0' && IsAllDigits(identifier))
            {
                throw new FormatException("invalid semantic version: numeric identifiers must not have leading zeros");
            }
        }

        return identifiers;
    }

    private static bool IsAllDigits(string value)
    {
        if (value.Length == 0)
        {
            return false;
        }

        foreach (char c in value)
        {
            if (c is < '0' or > '9')
            {
                return false;
            }
        }

        return true;
    }

    private static bool IsIdentifierChar(char c) => c is (>= '0' and <= '9') or (>= 'A' and <= 'Z') or (>= 'a' and <= 'z') or '-';

    private static int CompareIdentifiers(string left, string right)
    {
        bool leftNumeric = IsAllDigits(left);
        bool rightNumeric = IsAllDigits(right);
        if (leftNumeric && rightNumeric)
        {
            // numeric identifiers have no leading zeros, so length decides before digits do
            if (left.Length != right.Length)
            {
                return left.Length < right.Length ? -1 : 1;
            }

            return Math.Sign(string.CompareOrdinal(left, right));
        }

        if (leftNumeric)
        {
            return -1;
        }

        if (rightNumeric)
        {
            return 1;
        }

        return Math.Sign(string.CompareOrdinal(left, right));
    }
}
