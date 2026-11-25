from __future__ import annotations

import zlib

import pytest
from hypothesis import given
from hypothesis.strategies import binary, integers

from ciutil.adler32 import adler32, adler32_finalize, adler32_init, adler32_update
from helpers import lcg_bytes

VECTORS = [
    (b"", 0x00000001),
    (b"a", 0x00620062),
    (b"abc", 0x024D0127),
    (b"Wikipedia", 0x11E60398),
    (b"123456789", 0x091E01DE),
]


def naive_adler32(data: bytes) -> int:
    low = 1
    high = 0
    for byte in data:
        low = (low + byte) % 65521
        high = (high + low) % 65521
    return (high << 16) | low


def chunked_adler32(data: bytes, size: int) -> int:
    state = adler32_init()
    for start in range(0, len(data), size):
        state = adler32_update(state, data[start : start + size])
    return adler32_finalize(state)


@pytest.mark.parametrize(("data", "expected"), VECTORS)
def test_vectors(data: bytes, expected: int) -> None:
    assert adler32(data) == expected


def test_long_input_of_ff_bytes_matches_naive_reference() -> None:
    data = b"\xff" * 100000
    assert adler32(data) == naive_adler32(data)


@pytest.mark.parametrize("size", [1, 7, 5551, 5552, 5553, 65536])
def test_long_input_chunked_equals_one_shot(size: int) -> None:
    data = b"\xff" * 100000
    assert chunked_adler32(data, size) == adler32(data)


@pytest.mark.parametrize("length", [5551, 5552, 5553, 11104, 11105, 16657])
def test_block_boundaries_match_naive_reference(length: int) -> None:
    for data in (b"\xff" * length, lcg_bytes(length)):
        assert adler32(data) == naive_adler32(data)


def test_chunked_equals_one_shot_at_every_split_point() -> None:
    data = lcg_bytes(1000)
    expected = adler32(data)
    for split in range(len(data) + 1):
        state = adler32_init()
        state = adler32_update(state, data[:split])
        state = adler32_update(state, data[split:])
        assert adler32_finalize(state) == expected


@given(binary())
def test_matches_zlib(data: bytes) -> None:
    assert adler32(data) == zlib.adler32(data)


@given(integers(min_value=0, max_value=30000), integers(min_value=0, max_value=255))
def test_runs_of_one_byte_match_zlib_and_naive(length: int, value: int) -> None:
    data = bytes([value]) * length
    assert adler32(data) == zlib.adler32(data)
    assert adler32(data) == naive_adler32(data)


@given(binary(), binary())
def test_incremental_updates_equal_concatenation(left: bytes, right: bytes) -> None:
    state = adler32_update(adler32_update(adler32_init(), left), right)
    assert adler32_finalize(state) == adler32(left + right)
