from __future__ import annotations

_POLY = 0xEDB88320
_MASK = 0xFFFFFFFF


def _make_table() -> tuple[int, ...]:
    table: list[int] = []
    for index in range(256):
        value = index
        for _ in range(8):
            if value & 1:
                value = (value >> 1) ^ _POLY
            else:
                value >>= 1
        table.append(value)
    return tuple(table)


_TABLE = _make_table()


def crc32_init() -> int:
    return _MASK


def crc32_update(state: int, data: bytes | bytearray) -> int:
    for byte in data:
        state = _TABLE[(state ^ byte) & 0xFF] ^ (state >> 8)
    return state


def crc32_finalize(state: int) -> int:
    return state ^ _MASK


def crc32(data: bytes | bytearray) -> int:
    return crc32_finalize(crc32_update(crc32_init(), data))
