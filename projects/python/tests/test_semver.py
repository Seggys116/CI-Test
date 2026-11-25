from __future__ import annotations

import dataclasses
import functools

import pytest
from hypothesis import given
from hypothesis.strategies import SearchStrategy, builds, from_regex, integers, lists

from ciutil.semver import SemVerError, Version, compare, parse

CHAIN = [
    "1.0.0-alpha",
    "1.0.0-alpha.1",
    "1.0.0-alpha.beta",
    "1.0.0-beta",
    "1.0.0-beta.2",
    "1.0.0-beta.11",
    "1.0.0-rc.1",
    "1.0.0",
    "2.0.0",
    "2.1.0",
    "2.1.1",
]

VALID = [
    "0.0.0",
    "1.0.0",
    "1.0.0-0",
    "1.0.0-0a",
    "1.0.0-x-y-z.-",
    "1.0.0+001",
    "1.0.0-rc.1+build.5",
    "1.2.3-alpha.1.2.3+exp.sha.5114f85",
    "10.20.30",
    "1.0.0-alpha-beta",
    "1.0.0+a-b",
    "18446744073709551615.18446744073709551615.18446744073709551615",
]

INVALID = [
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
    "1.0.0-alpha.01",
    "1.0.0-a..b",
    "1.0.0-.a",
    "1.0.0-a.",
    "1.0.0+a..b",
    "1.0.0+a_b",
    "1.0.0-a_b",
    "1.0.0+a+b",
    "-1.0.0",
    "+1.0.0",
    " 1.0.0",
    "1.0.0 ",
    "1.0.0\n",
    "1.0.0-alpha+",
    "a.b.c",
    "1.0.x",
    "1..0",
    "١.0.0",
    "1.0.0-é",
    "18446744073709551616.0.0",
    "0.18446744073709551616.0",
    "0.0.99999999999999999999999999",
]

_PRERELEASE_ID = from_regex(
    r"0|[1-9][0-9]{0,3}|[0-9A-Za-z-]*[A-Za-z-][0-9A-Za-z-]*",
    fullmatch=True,
)
_BUILD_ID = from_regex(r"[0-9A-Za-z-]+", fullmatch=True)

VERSIONS: SearchStrategy[Version] = builds(
    Version,
    major=integers(0, 3),
    minor=integers(0, 3),
    patch=integers(0, 3),
    prerelease=lists(_PRERELEASE_ID, max_size=4).map(tuple),
    build=lists(_BUILD_ID, max_size=2).map(tuple),
)


@pytest.mark.parametrize("candidate", VALID)
def test_parse_accepts_and_round_trips(candidate: str) -> None:
    assert str(parse(candidate)) == candidate


@pytest.mark.parametrize("candidate", INVALID)
def test_parse_rejects(candidate: str) -> None:
    with pytest.raises(SemVerError):
        parse(candidate)


def test_parse_extracts_all_fields() -> None:
    expected = Version(1, 2, 3, ("rc", "1"), ("build", "5"))
    assert parse("1.2.3-rc.1+build.5") == expected


def test_parse_keeps_dashes_inside_identifiers() -> None:
    assert parse("1.0.0-x-y-z.-").prerelease == ("x-y-z", "-")


def test_parse_keeps_leading_zeros_in_build_identifiers() -> None:
    assert parse("1.0.0+001").build == ("001",)


def test_parse_accepts_u64_maximum() -> None:
    version = parse("18446744073709551615.0.0")
    assert version.major == 18446744073709551615


def test_chain_is_strictly_increasing_for_all_ordered_pairs() -> None:
    versions = [parse(item) for item in CHAIN]
    for i, left in enumerate(versions):
        for j, right in enumerate(versions):
            expected = (i > j) - (i < j)
            assert compare(left, right) == expected, (CHAIN[i], CHAIN[j])


@pytest.mark.parametrize(
    ("left", "right", "expected"),
    [
        ("1.0.0+a", "1.0.0+b", 0),
        ("1.0.0+a", "1.0.0", 0),
        ("1.0.0-rc.1+x", "1.0.0-rc.1+y", 0),
        ("1.0.0-beta.2", "1.0.0-beta.11", -1),
        ("1.0.0-1", "1.0.0-a", -1),
        ("1.0.0-a", "1.0.0-B", 1),
        ("1.0.0-alpha", "1.0.0-alpha.1", -1),
        ("1.0.0-0a", "1.0.0-1", 1),
        ("1.0.0-99999999999999999999", "1.0.0-100000000000000000000", -1),
        ("1.9.0", "1.10.0", -1),
        ("2.0.0", "1.99.99", 1),
    ],
)
def test_compare_cases(left: str, right: str, expected: int) -> None:
    assert compare(parse(left), parse(right)) == expected
    assert compare(parse(right), parse(left)) == -expected


def test_parse_of_formatted_chain_round_trips() -> None:
    for item in CHAIN:
        assert str(parse(item)) == item


@given(VERSIONS)
def test_compare_is_reflexive(version: Version) -> None:
    assert compare(version, version) == 0


@given(VERSIONS, VERSIONS)
def test_compare_is_antisymmetric(left: Version, right: Version) -> None:
    assert compare(left, right) == -compare(right, left)


@given(VERSIONS, VERSIONS, VERSIONS)
def test_compare_is_transitive(a: Version, b: Version, c: Version) -> None:
    ordered = sorted([a, b, c], key=functools.cmp_to_key(compare))
    assert compare(ordered[0], ordered[1]) <= 0
    assert compare(ordered[1], ordered[2]) <= 0
    assert compare(ordered[0], ordered[2]) <= 0


@given(VERSIONS, VERSIONS, VERSIONS)
def test_strict_order_is_transitive(a: Version, b: Version, c: Version) -> None:
    if compare(a, b) < 0 and compare(b, c) < 0:
        assert compare(a, c) < 0
    if compare(a, b) == 0 and compare(b, c) == 0:
        assert compare(a, c) == 0


@given(VERSIONS, lists(_BUILD_ID, max_size=3).map(tuple))
def test_build_metadata_is_ignored(version: Version, build: tuple[str, ...]) -> None:
    assert compare(version, dataclasses.replace(version, build=build)) == 0


@given(VERSIONS)
def test_str_round_trips_through_parse(version: Version) -> None:
    assert parse(str(version)) == version


def _precedence_key(version: Version) -> tuple[int, int, int, tuple[str, ...]]:
    return (version.major, version.minor, version.patch, version.prerelease)


@given(VERSIONS, VERSIONS)
def test_zero_means_equal_ignoring_build(left: Version, right: Version) -> None:
    same = _precedence_key(left) == _precedence_key(right)
    assert (compare(left, right) == 0) == same
