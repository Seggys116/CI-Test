from __future__ import annotations

_TABLE = (
    (1000, "M"),
    (900, "CM"),
    (500, "D"),
    (400, "CD"),
    (100, "C"),
    (90, "XC"),
    (50, "L"),
    (40, "XL"),
    (10, "X"),
    (9, "IX"),
    (5, "V"),
    (4, "IV"),
    (1, "I"),
)


class RomanError(ValueError):
    pass


def to_roman(number: int) -> str:
    if number < 1 or number > 3999:
        raise RomanError(f"number out of range 1..3999: {number}")
    parts: list[str] = []
    for value, symbol in _TABLE:
        while number >= value:
            parts.append(symbol)
            number -= value
    return "".join(parts)


def from_roman(text: str) -> int:
    pos = 0
    total = 0
    for value, symbol in _TABLE:
        while text.startswith(symbol, pos):
            total += value
            pos += len(symbol)
    if pos != len(text) or total == 0 or to_roman(total) != text:
        raise RomanError(f"invalid roman numeral: {text!r}")
    return total
