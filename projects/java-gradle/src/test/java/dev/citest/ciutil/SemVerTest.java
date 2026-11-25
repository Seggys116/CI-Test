package dev.citest.ciutil;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.ArrayList;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

class SemVerTest {

    private static final List<String> CHAIN =
            List.of(
                    "1.0.0-alpha",
                    "1.0.0-alpha.1",
                    "1.0.0-alpha.beta",
                    "1.0.0-beta",
                    "1.0.0-beta.2",
                    "1.0.0-beta.11",
                    "1.0.0-rc.1",
                    "1.0.0");

    private static final List<String> CORE_CHAIN = List.of("1.0.0", "2.0.0", "2.1.0", "2.1.1");

    private static final List<String> ACCEPTED =
            List.of(
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
                    "9223372036854775807.9223372036854775807.9223372036854775807");

    @ParameterizedTest
    @ValueSource(
            strings = {
                "",
                "1",
                "1.0",
                "1.0.0.0",
                "v1.0.0",
                "01.0.0",
                "1.00.0",
                "1.0.00",
                "1.0.0-",
                "1.0.0+",
                "1.0.0-01",
                "1.0.0-a..b",
                "1.0.0-a_b",
                "-1.0.0",
                " 1.0.0",
                "1.0.0 ",
                "1.0.0-alpha+",
                "1.0.0-alpha.",
                "1.0.0-.alpha",
                "1.0.0+a..b",
                "1.0.0+a_b",
                "1.0.0+a+b",
                "1.0.0\n",
                "1..0",
                "1.0.",
                ".1.0",
                "a.b.c",
                "1.0.x",
                "1.0.0-alpha.01",
                "+1.0.0",
                "1.0.0-é",
                "١.0.0",
                "9223372036854775808.0.0",
                "1.9223372036854775808.0",
                "1.0.99999999999999999999",
            })
    void rejectsInvalidVersions(String text) {
        assertThrows(IllegalArgumentException.class, () -> SemVer.parse(text));
    }

    @ParameterizedTest
    @ValueSource(
            strings = {
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
                "9223372036854775807.9223372036854775807.9223372036854775807",
            })
    void acceptsValidVersionsAndRoundTripsThroughToString(String text) {
        SemVer version = SemVer.parse(text);
        assertEquals(text, version.toString());
        assertEquals(version, SemVer.parse(version.toString()));
    }

    @Test
    void parsesAllComponents() {
        SemVer version = SemVer.parse("1.2.3-rc.1+build.5");
        assertEquals(1, version.major());
        assertEquals(2, version.minor());
        assertEquals(3, version.patch());
        assertEquals(List.of("rc", "1"), version.prerelease());
        assertEquals(List.of("build", "5"), version.build());
    }

    @Test
    void parsesPlainVersion() {
        SemVer version = SemVer.parse("0.0.0");
        assertEquals(new SemVer(0, 0, 0, List.of(), List.of()), version);
        assertTrue(version.prerelease().isEmpty());
        assertTrue(version.build().isEmpty());
    }

    @Test
    void hyphensInsideIdentifiersAreKept() {
        assertEquals(List.of("x-y-z", "-"), SemVer.parse("1.0.0-x-y-z.-").prerelease());
    }

    @Test
    void orderingChainHoldsForAllOrderedPairs() {
        List<SemVer> versions = CHAIN.stream().map(SemVer::parse).toList();
        for (int i = 0; i < versions.size(); i++) {
            assertEquals(0, SemVer.compare(versions.get(i), versions.get(i)), CHAIN.get(i));
            for (int j = i + 1; j < versions.size(); j++) {
                String pair = CHAIN.get(i) + " vs " + CHAIN.get(j);
                assertEquals(-1, SemVer.compare(versions.get(i), versions.get(j)), pair);
                assertEquals(1, SemVer.compare(versions.get(j), versions.get(i)), pair);
            }
        }
    }

    @Test
    void coreOrderingHoldsForAllOrderedPairs() {
        List<SemVer> versions = CORE_CHAIN.stream().map(SemVer::parse).toList();
        for (int i = 0; i < versions.size(); i++) {
            for (int j = i + 1; j < versions.size(); j++) {
                String pair = CORE_CHAIN.get(i) + " vs " + CORE_CHAIN.get(j);
                assertEquals(-1, SemVer.compare(versions.get(i), versions.get(j)), pair);
                assertEquals(1, SemVer.compare(versions.get(j), versions.get(i)), pair);
            }
        }
    }

    @Test
    void compareIsAntisymmetricOverAllSamples() {
        List<String> samples = new ArrayList<>(CHAIN);
        samples.addAll(CORE_CHAIN);
        samples.addAll(ACCEPTED);
        for (String left : samples) {
            for (String right : samples) {
                int forward = SemVer.compare(SemVer.parse(left), SemVer.parse(right));
                int backward = SemVer.compare(SemVer.parse(right), SemVer.parse(left));
                assertEquals(-backward, forward, left + " vs " + right);
            }
        }
    }

    @Test
    void buildMetadataIsIgnored() {
        assertEquals(0, SemVer.compare(SemVer.parse("1.0.0+a"), SemVer.parse("1.0.0+b")));
        assertEquals(
                0, SemVer.compare(SemVer.parse("1.0.0-rc.1+x"), SemVer.parse("1.0.0-rc.1")));
    }

    @Test
    void numericIdentifiersCompareNumerically() {
        assertEquals(
                -1, SemVer.compare(SemVer.parse("1.0.0-beta.2"), SemVer.parse("1.0.0-beta.11")));
    }

    @Test
    void hugeNumericIdentifiersCompareWithoutOverflow() {
        SemVer smaller = SemVer.parse("1.0.0-99999999999999999999999999");
        SemVer larger = SemVer.parse("1.0.0-100000000000000000000000000");
        assertEquals(-1, SemVer.compare(smaller, larger));
        assertEquals(1, SemVer.compare(larger, smaller));
    }

    @Test
    void numericIdentifiersSortBeforeAlphanumeric() {
        SemVer numeric = SemVer.parse("1.0.0-99999");
        SemVer alphanumeric = SemVer.parse("1.0.0-a");
        assertEquals(-1, SemVer.compare(numeric, alphanumeric));
        assertEquals(1, SemVer.compare(alphanumeric, numeric));
    }

    @Test
    void alphanumericIdentifiersCompareInAsciiOrder() {
        assertEquals(-1, SemVer.compare(SemVer.parse("1.0.0-Z"), SemVer.parse("1.0.0-a")));
        assertEquals(-1, SemVer.compare(SemVer.parse("1.0.0-a-"), SemVer.parse("1.0.0-aa")));
    }

    @Test
    void longerPrereleaseListWinsWhenPrefixIsEqual() {
        assertEquals(
                -1, SemVer.compare(SemVer.parse("1.0.0-alpha"), SemVer.parse("1.0.0-alpha.0")));
    }

    @Test
    void releaseOutranksPrerelease() {
        assertEquals(1, SemVer.compare(SemVer.parse("2.0.0"), SemVer.parse("2.0.0-rc.1")));
    }

    @Test
    void prereleaseOfALaterVersionOutranksEarlierRelease() {
        assertEquals(-1, SemVer.compare(SemVer.parse("1.9.9"), SemVer.parse("2.0.0-alpha")));
    }
}
