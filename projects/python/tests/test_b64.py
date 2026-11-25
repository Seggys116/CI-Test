from __future__ import annotations

import base64
import contextlib

import pytest
from hypothesis import given
from hypothesis.strategies import binary, text

from ciutil.b64 import Base64Error, decode, encode
from helpers import lcg_bytes

ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

VECTORS = [
    (b"", ""),
    (b"f", "Zg=="),
    (b"fo", "Zm8="),
    (b"foo", "Zm9v"),
    (b"foob", "Zm9vYg=="),
    (b"fooba", "Zm9vYmE="),
    (b"foobar", "Zm9vYmFy"),
    (b"\x00\xff\xfe", "AP/+"),
]

REJECTED = [
    "A",
    "Zg",
    "Zg=",
    "AAA",
    "Zm9vY",
    "Zm9v\n",
    "Zm-v",
    "Zm_v",
    "AP_-",
    "-_-_",
    "Zm9\n",
    "Zm9\r",
    "Zm 9",
    " Zm9",
    "Zm9 ",
    "Zm9\t",
    "Zm9é",
    "éééé",
    "Zg==Zg==",
    "Zg==Zm9v",
    "Zg=A",
    "=Zg=",
    "Z=g=",
    "A=AA",
    "=AAA",
    "Z===",
    "====",
    "Zm9v====",
    "Zg===",
    "Zh==",
    "Zm9=",
    "ZH==",
    "AAB=",
]


@pytest.mark.parametrize(("data", "expected"), VECTORS)
def test_encode_vectors(data: bytes, expected: str) -> None:
    assert encode(data) == expected


@pytest.mark.parametrize(("expected", "text_in"), VECTORS)
def test_decode_vectors(expected: bytes, text_in: str) -> None:
    assert decode(text_in) == expected


def test_encode_accepts_bytearray() -> None:
    assert encode(bytearray(b"foobar")) == "Zm9vYmFy"


@pytest.mark.parametrize("candidate", REJECTED)
def test_decode_rejects(candidate: str) -> None:
    with pytest.raises(Base64Error):
        decode(candidate)


def test_round_trip_for_every_length_up_to_300() -> None:
    source = lcg_bytes(300)
    for length in range(301):
        data = source[:length]
        assert decode(encode(data)) == data


@given(binary())
def test_round_trip(data: bytes) -> None:
    assert decode(encode(data)) == data


@given(binary())
def test_encode_matches_stdlib(data: bytes) -> None:
    assert encode(data) == base64.b64encode(data).decode("ascii")


@given(binary())
def test_decode_matches_stdlib_for_canonical_input(data: bytes) -> None:
    encoded = base64.b64encode(data)
    assert decode(encoded.decode("ascii")) == base64.b64decode(encoded, validate=True)


@given(text(alphabet=ALPHABET + "=", max_size=12))
def test_accepted_input_is_canonical(candidate: str) -> None:
    try:
        decoded = decode(candidate)
    except Base64Error:
        return
    assert encode(decoded) == candidate


@given(text(max_size=40))
def test_arbitrary_text_only_raises_base64_error(candidate: str) -> None:
    with contextlib.suppress(Base64Error):
        decode(candidate)
