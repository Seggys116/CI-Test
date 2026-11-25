from __future__ import annotations

_MOD = 65521
_NMAX = 5552


def adler32_init() -> int:
    return 1


def adler32_update(state: int, data: bytes | bytearray) -> int:
    low = state & 0xFFFF
    high = (state >> 16) & 0xFFFF
    for start in range(0, len(data), _NMAX):
        for byte in data[start : start + _NMAX]:
            low += byte
            high += low
        low %= _MOD
        high %= _MOD
    return (high << 16) | low


def adler32_finalize(state: int) -> int:
    return state


def adler32(data: bytes | bytearray) -> int:
    return adler32_finalize(adler32_update(adler32_init(), data))
