from __future__ import annotations

import binascii
import zlib

import pytest
from hypothesis import given
from hypothesis.strategies import binary

from ciutil.crc32 import crc32, crc32_finalize, crc32_init, crc32_update
from helpers import lcg_bytes

VECTORS = [
    (b"", 0x00000000),
    (b"a", 0xE8B7BE43),
    (b"abc", 0x352441C2),
    (b"123456789", 0xCBF43926),
    (b"The quick brown fox jumps over the lazy dog", 0x414FA339),
    (bytes(32), 0x190A55AD),
    (b"\xff" * 32, 0xFF6CAB0B),
]


@pytest.mark.parametrize(("data", "expected"), VECTORS)
def test_vectors(data: bytes, expected: int) -> None:
    assert crc32(data) == expected


def test_accepts_bytearray() -> None:
    assert crc32(bytearray(b"123456789")) == 0xCBF43926


def test_chunked_equals_one_shot_at_every_split_point() -> None:
    data = lcg_bytes(1000)
    expected = crc32(data)
    for split in range(len(data) + 1):
        state = crc32_init()
        state = crc32_update(state, data[:split])
        state = crc32_update(state, data[split:])
        assert crc32_finalize(state) == expected


def test_byte_at_a_time_equals_one_shot() -> None:
    data = lcg_bytes(1000)
    state = crc32_init()
    for byte in data:
        state = crc32_update(state, bytes([byte]))
    assert crc32_finalize(state) == crc32(data)


@given(binary())
def test_matches_zlib(data: bytes) -> None:
    assert crc32(data) == zlib.crc32(data)


@given(binary())
def test_matches_binascii(data: bytes) -> None:
    assert crc32(data) == binascii.crc32(data)


@given(binary(), binary())
def test_concatenation_via_incremental_updates(left: bytes, right: bytes) -> None:
    state = crc32_update(crc32_update(crc32_init(), left), right)
    assert crc32_finalize(state) == crc32(left + right)


@given(binary())
def test_result_fits_in_u32(data: bytes) -> None:
    assert 0 <= crc32(data) <= 0xFFFFFFFF
