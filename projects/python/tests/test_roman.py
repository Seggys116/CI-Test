from __future__ import annotations

import itertools
import re

import pytest
from hypothesis import given
from hypothesis.strategies import integers, text

from ciutil.roman import RomanError, from_roman, to_roman

CANONICAL = re.compile(r"M{0,3}(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})")

VECTORS = [
    (1, "I"),
    (4, "IV"),
    (9, "IX"),
    (14, "XIV"),
    (40, "XL"),
    (90, "XC"),
    (400, "CD"),
    (1994, "MCMXCIV"),
    (2024, "MMXXIV"),
    (3999, "MMMCMXCIX"),
]

INVALID_NUMERALS = [
    "",
    "IIII",
    "VX",
    "IC",
    "XM",
    "VV",
    "IIX",
    "xiv",
    "MMMM",
    "IXI",
    "VIV",
    "CMC",
    "IL",
    "XD",
    "XXXX",
    "LL",
    "DD",
    "IVI",
    " X",
    "X ",
    "X\n",
    "A",
    "Ⅻ",
]


@pytest.mark.parametrize(("number", "numeral"), VECTORS)
def test_to_roman_vectors(number: int, numeral: str) -> None:
    assert to_roman(number) == numeral


@pytest.mark.parametrize(("number", "numeral"), VECTORS)
def test_from_roman_vectors(number: int, numeral: str) -> None:
    assert from_roman(numeral) == number


@pytest.mark.parametrize("number", [0, -1, -3999, 4000, 4001, 10**6])
def test_to_roman_rejects_out_of_range(number: int) -> None:
    with pytest.raises(RomanError):
        to_roman(number)


@pytest.mark.parametrize("numeral", INVALID_NUMERALS)
def test_from_roman_rejects(numeral: str) -> None:
    with pytest.raises(RomanError):
        from_roman(numeral)


def test_exhaustive_round_trip() -> None:
    seen: set[str] = set()
    for number in range(1, 4000):
        numeral = to_roman(number)
        assert from_roman(numeral) == number
        assert CANONICAL.fullmatch(numeral)
        seen.add(numeral)
    assert len(seen) == 3999


def test_from_roman_accepts_exactly_the_canonical_forms_up_to_length_four() -> None:
    for length in range(1, 5):
        for symbols in itertools.product("IVXLCDM", repeat=length):
            candidate = "".join(symbols)
            try:
                number = from_roman(candidate)
            except RomanError:
                assert not CANONICAL.fullmatch(candidate)
            else:
                assert CANONICAL.fullmatch(candidate)
                assert to_roman(number) == candidate


@given(integers(min_value=1, max_value=3999))
def test_round_trip_property(number: int) -> None:
    assert from_roman(to_roman(number)) == number


@given(text(alphabet="IVXLCDM", max_size=12))
def test_accepted_numerals_are_canonical(candidate: str) -> None:
    try:
        number = from_roman(candidate)
    except RomanError:
        return
    assert to_roman(number) == candidate
