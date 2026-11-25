from __future__ import annotations

_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
_DECODE = {char: index for index, char in enumerate(_ALPHABET)}


class Base64Error(ValueError):
    pass


def encode(data: bytes | bytearray) -> str:
    out: list[str] = []
    full = len(data) - len(data) % 3
    for i in range(0, full, 3):
        block = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2]
        out.append(_ALPHABET[block >> 18])
        out.append(_ALPHABET[(block >> 12) & 0x3F])
        out.append(_ALPHABET[(block >> 6) & 0x3F])
        out.append(_ALPHABET[block & 0x3F])
    remaining = len(data) - full
    if remaining == 1:
        block = data[full] << 16
        out.append(_ALPHABET[block >> 18])
        out.append(_ALPHABET[(block >> 12) & 0x3F])
        out.append("==")
    elif remaining == 2:
        block = (data[full] << 16) | (data[full + 1] << 8)
        out.append(_ALPHABET[block >> 18])
        out.append(_ALPHABET[(block >> 12) & 0x3F])
        out.append(_ALPHABET[(block >> 6) & 0x3F])
        out.append("=")
    return "".join(out)


def decode(text: str) -> bytes:
    if len(text) % 4 != 0:
        raise Base64Error("length is not a multiple of 4")
    if text.endswith("=="):
        pad = 2
    elif text.endswith("="):
        pad = 1
    else:
        pad = 0
    values: list[int] = []
    for char in text[: len(text) - pad]:
        value = _DECODE.get(char)
        if value is None:
            raise Base64Error(f"invalid character {char!r}")
        values.append(value)
    out = bytearray()
    for i in range(0, len(values), 4):
        chunk = values[i : i + 4]
        if len(chunk) == 4:
            block = (chunk[0] << 18) | (chunk[1] << 12) | (chunk[2] << 6) | chunk[3]
            out.append((block >> 16) & 0xFF)
            out.append((block >> 8) & 0xFF)
            out.append(block & 0xFF)
        elif len(chunk) == 3:
            if chunk[2] & 0x03:
                raise Base64Error("non-canonical trailing bits")
            block = (chunk[0] << 18) | (chunk[1] << 12) | (chunk[2] << 6)
            out.append((block >> 16) & 0xFF)
            out.append((block >> 8) & 0xFF)
        else:
            if chunk[1] & 0x0F:
                raise Base64Error("non-canonical trailing bits")
            block = (chunk[0] << 18) | (chunk[1] << 12)
            out.append((block >> 16) & 0xFF)
    return bytes(out)
