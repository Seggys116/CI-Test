package dev.citest.ciutil;

import java.util.List;

/**
 * a Semantic Versioning 2.0.0 version. numeric fields are limited to 63 bits. {@code equals}
 * is structural and includes build metadata; {@link #compare} follows precedence and ignores it.
 */
public record SemVer(
        long major, long minor, long patch, List<String> prerelease, List<String> build) {

    public SemVer {
        prerelease = List.copyOf(prerelease);
        build = List.copyOf(build);
    }

    /**
     * @throws IllegalArgumentException if the text is not a valid semantic version
     */
    public static SemVer parse(String text) {
        String core = text;

        List<String> build = List.of();
        int plus = core.indexOf('+');
        if (plus >= 0) {
            build = parseIdentifiers(core.substring(plus + 1), true);
            core = core.substring(0, plus);
        }

        List<String> prerelease = List.of();
        int dash = core.indexOf('-');
        if (dash >= 0) {
            prerelease = parseIdentifiers(core.substring(dash + 1), false);
            core = core.substring(0, dash);
        }

        String[] fields = core.split("\\.", -1);
        if (fields.length != 3) {
            throw invalid(text, "expected MAJOR.MINOR.PATCH");
        }
        return new SemVer(
                parseNumber(fields[0], text),
                parseNumber(fields[1], text),
                parseNumber(fields[2], text),
                prerelease,
                build);
    }

    /** returns -1, 0 or 1 by semver precedence; build metadata is ignored. */
    public static int compare(SemVer a, SemVer b) {
        int result = Long.compare(a.major, b.major);
        if (result == 0) {
            result = Long.compare(a.minor, b.minor);
        }
        if (result == 0) {
            result = Long.compare(a.patch, b.patch);
        }
        if (result != 0) {
            return Integer.signum(result);
        }

        if (a.prerelease.isEmpty() || b.prerelease.isEmpty()) {
            if (a.prerelease.isEmpty() && b.prerelease.isEmpty()) {
                return 0;
            }
            // a release outranks any of its prereleases
            return a.prerelease.isEmpty() ? 1 : -1;
        }

        int shared = Math.min(a.prerelease.size(), b.prerelease.size());
        for (int i = 0; i < shared; i++) {
            int identifier = compareIdentifiers(a.prerelease.get(i), b.prerelease.get(i));
            if (identifier != 0) {
                return identifier;
            }
        }
        return Integer.compare(a.prerelease.size(), b.prerelease.size());
    }

    @Override
    public String toString() {
        StringBuilder out = new StringBuilder();
        out.append(major).append('.').append(minor).append('.').append(patch);
        if (!prerelease.isEmpty()) {
            out.append('-').append(String.join(".", prerelease));
        }
        if (!build.isEmpty()) {
            out.append('+').append(String.join(".", build));
        }
        return out.toString();
    }

    private static IllegalArgumentException invalid(String text, String reason) {
        return new IllegalArgumentException("invalid semantic version \"" + text + "\": " + reason);
    }

    private static long parseNumber(String field, String text) {
        if (field.isEmpty() || !isAllDigits(field)) {
            throw invalid(text, "version fields must be non-negative integers");
        }
        if (field.length() > 1 && field.charAt(0) == '0') {
            throw invalid(text, "version fields must not have leading zeros");
        }
        long value = 0;
        for (int i = 0; i < field.length(); i++) {
            int digit = field.charAt(i) - '0';
            if (value > (Long.MAX_VALUE - digit) / 10) {
                throw invalid(text, "version field does not fit in 63 bits");
            }
            value = value * 10 + digit;
        }
        return value;
    }

    private static List<String> parseIdentifiers(String part, boolean allowLeadingZeros) {
        String[] identifiers = part.split("\\.", -1);
        for (String identifier : identifiers) {
            if (identifier.isEmpty()) {
                throw new IllegalArgumentException("invalid semantic version: empty identifier");
            }
            for (int i = 0; i < identifier.length(); i++) {
                if (!isIdentifierChar(identifier.charAt(i))) {
                    throw new IllegalArgumentException(
                            "invalid semantic version: identifiers may only contain [0-9A-Za-z-]");
                }
            }
            if (!allowLeadingZeros
                    && identifier.length() > 1
                    && identifier.charAt(0) == '0'
                    && isAllDigits(identifier)) {
                throw new IllegalArgumentException(
                        "invalid semantic version: numeric identifiers must not have leading zeros");
            }
        }
        return List.of(identifiers);
    }

    private static boolean isAllDigits(String value) {
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if (c < '0' || c > '9') {
                return false;
            }
        }
        return !value.isEmpty();
    }

    private static boolean isIdentifierChar(char c) {
        return (c >= '0' && c <= '9')
                || (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || c == '-';
    }

    private static int compareIdentifiers(String a, String b) {
        boolean numericA = isAllDigits(a);
        boolean numericB = isAllDigits(b);
        if (numericA && numericB) {
            // numeric identifiers have no leading zeros, so length decides before digits do
            if (a.length() != b.length()) {
                return a.length() < b.length() ? -1 : 1;
            }
            return Integer.signum(a.compareTo(b));
        }
        if (numericA) {
            return -1;
        }
        if (numericB) {
            return 1;
        }
        return Integer.signum(a.compareTo(b));
    }
}
