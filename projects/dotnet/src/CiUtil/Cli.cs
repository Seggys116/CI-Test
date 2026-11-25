using System.Globalization;
using System.Text;

namespace CiUtil;

/// <summary>Command line front end, kept free of Console so tests can drive it with plain streams.</summary>
public static class Cli
{
    public const int ExitOk = 0;
    public const int ExitError = 2;
    public const int ExitUsage = 64;

    private const string Usage =
        "usage: ciutil <command> [arguments]\n" +
        "commands:\n" +
        "  crc32 STRING\n" +
        "  adler32 STRING\n" +
        "  b64enc STRING\n" +
        "  b64dec BASE64\n" +
        "  roman NUMBER\n" +
        "  unroman ROMAN\n" +
        "  semver-cmp A B\n" +
        "  version\n";

    /// <summary>
    /// Runs one invocation. <paramref name="args"/> excludes the program name; output is written as raw bytes.
    /// </summary>
    /// <returns>The process exit code: 0 on success, 2 on error, 64 on misuse.</returns>
    public static int Run(string[] args, Stream stdout, Stream stderr)
    {
        ArgumentNullException.ThrowIfNull(args);
        ArgumentNullException.ThrowIfNull(stdout);
        ArgumentNullException.ThrowIfNull(stderr);

        int code;
        try
        {
            code = Dispatch(args, stdout, stderr);
        }
        catch (ArgumentOutOfRangeException)
        {
            WriteText(stderr, "ciutil: number must be between 1 and 3999\n");
            code = ExitError;
        }
        catch (FormatException ex)
        {
            WriteText(stderr, $"ciutil: {ex.Message}\n");
            code = ExitError;
        }

        stdout.Flush();
        stderr.Flush();
        return code;
    }

    private static int Dispatch(string[] args, Stream stdout, Stream stderr)
    {
        if (args.Length == 0)
        {
            return Fail(stderr);
        }

        switch (args[0], args.Length - 1)
        {
            case ("version", 0):
                WriteText(stdout, "ciutil dotnet 1.0.0\n");
                return ExitOk;
            case ("crc32", 1):
                WriteText(stdout, Hex32(Crc32.Compute(Utf8(args[1]))) + "\n");
                return ExitOk;
            case ("adler32", 1):
                WriteText(stdout, Hex32(Adler32.Compute(Utf8(args[1]))) + "\n");
                return ExitOk;
            case ("b64enc", 1):
                WriteText(stdout, Base64.Encode(Utf8(args[1])) + "\n");
                return ExitOk;
            case ("b64dec", 1):
                stdout.Write(Base64.Decode(args[1]));
                return ExitOk;
            case ("roman", 1):
                WriteText(stdout, Roman.ToRoman(ParseDecimal(args[1])) + "\n");
                return ExitOk;
            case ("unroman", 1):
                WriteText(stdout, Roman.FromRoman(args[1]).ToString(CultureInfo.InvariantCulture) + "\n");
                return ExitOk;
            case ("semver-cmp", 2):
                int order = SemVer.Compare(SemVer.Parse(args[1]), SemVer.Parse(args[2]));
                WriteText(stdout, order.ToString(CultureInfo.InvariantCulture) + "\n");
                return ExitOk;
            default:
                return Fail(stderr);
        }
    }

    private static int Fail(Stream stderr)
    {
        WriteText(stderr, Usage);
        return ExitUsage;
    }

    private static byte[] Utf8(string text) => Encoding.UTF8.GetBytes(text);

    private static void WriteText(Stream stream, string text) => stream.Write(Utf8(text));

    private static string Hex32(uint value) => value.ToString("x8", CultureInfo.InvariantCulture);

    private static int ParseDecimal(string text)
    {
        if (!int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out int value))
        {
            throw new FormatException($"not a non-negative decimal integer: {text}");
        }

        return value;
    }
}
