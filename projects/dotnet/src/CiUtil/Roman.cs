using System.Text;

namespace CiUtil;

/// <summary>Roman numerals for 1 to 3999; parsing accepts only the canonical spelling.</summary>
public static class Roman
{
    private static readonly int[] Values = [1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1];

    private static readonly string[] Symbols = ["M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"];

    /// <exception cref="ArgumentOutOfRangeException">The number is outside 1 to 3999.</exception>
    public static string ToRoman(int number)
    {
        if (number is < 1 or > 3999)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "number must be between 1 and 3999");
        }

        int remaining = number;
        var output = new StringBuilder();
        for (int i = 0; i < Values.Length; i++)
        {
            while (remaining >= Values[i])
            {
                output.Append(Symbols[i]);
                remaining -= Values[i];
            }
        }

        return output.ToString();
    }

    /// <exception cref="FormatException">The text is not a canonical roman numeral.</exception>
    public static int FromRoman(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        if (text.Length == 0)
        {
            throw new FormatException("empty roman numeral");
        }

        var digits = new int[text.Length];
        for (int i = 0; i < digits.Length; i++)
        {
            digits[i] = SymbolValue(text[i]);
            if (digits[i] == 0)
            {
                throw new FormatException($"invalid roman numeral: {text}");
            }
        }

        int total = 0;
        for (int i = 0; i < digits.Length; i++)
        {
            if (i + 1 < digits.Length && digits[i] < digits[i + 1])
            {
                total -= digits[i];
            }
            else
            {
                total += digits[i];
            }
        }

        // the canonical spelling of a number is unique, so re-encoding rejects every other form
        if (total is < 1 or > 3999 || ToRoman(total) != text)
        {
            throw new FormatException($"invalid roman numeral: {text}");
        }

        return total;
    }

    private static int SymbolValue(char symbol) => symbol switch
    {
        'I' => 1,
        'V' => 5,
        'X' => 10,
        'L' => 50,
        'C' => 100,
        'D' => 500,
        'M' => 1000,
        _ => 0,
    };
}
