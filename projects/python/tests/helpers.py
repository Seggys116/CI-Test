from __future__ import annotations


def lcg_bytes(count: int, seed: int = 42) -> bytes:
    state = seed
    out = bytearray()
    for _ in range(count):
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        out.append((state >> 16) & 0xFF)
    return bytes(out)
