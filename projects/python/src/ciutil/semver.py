from __future__ import annotations

import string
from dataclasses import dataclass

_U64_MAX = 0xFFFF_FFFF_FFFF_FFFF
_U64_MAX_DIGITS = 20
_IDENTIFIER_CHARS = frozenset(string.ascii_letters + string.digits + "-")


class SemVerError(ValueError):
    pass


@dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int
    prerelease: tuple[str, ...] = ()
    build: tuple[str, ...] = ()

    def __str__(self) -> str:
        text = f"{self.major}.{self.minor}.{self.patch}"
        if self.prerelease:
            text += "-" + ".".join(self.prerelease)
        if self.build:
            text += "+" + ".".join(self.build)
        return text


def _is_numeric(text: str) -> bool:
    return text.isascii() and text.isdigit()


def _parse_number(part: str) -> int:
    if not _is_numeric(part):
        raise SemVerError(f"invalid numeric component {part!r}")
    if len(part) > 1 and part.startswith("0"):
        raise SemVerError(f"leading zero in {part!r}")
    if len(part) > _U64_MAX_DIGITS or int(part) > _U64_MAX:
        raise SemVerError(f"number exceeds 64 bits: {part!r}")
    return int(part)


def _parse_identifiers(text: str, *, prerelease: bool) -> tuple[str, ...]:
    identifiers = text.split(".")
    for identifier in identifiers:
        if not identifier or not set(identifier) <= _IDENTIFIER_CHARS:
            raise SemVerError(f"invalid identifier {identifier!r}")
        numeric = _is_numeric(identifier)
        if prerelease and numeric and len(identifier) > 1 and identifier[0] == "0":
            raise SemVerError(f"leading zero in identifier {identifier!r}")
    return tuple(identifiers)


def parse(text: str) -> Version:
    head, has_build, build_text = text.partition("+")
    core, has_prerelease, prerelease_text = head.partition("-")
    parts = core.split(".")
    if len(parts) != 3:
        raise SemVerError(f"expected MAJOR.MINOR.PATCH in {text!r}")
    major, minor, patch = (_parse_number(part) for part in parts)
    prerelease: tuple[str, ...] = ()
    if has_prerelease:
        prerelease = _parse_identifiers(prerelease_text, prerelease=True)
    build: tuple[str, ...] = ()
    if has_build:
        build = _parse_identifiers(build_text, prerelease=False)
    return Version(major, minor, patch, prerelease, build)


def _sign(left: int, right: int) -> int:
    return (left > right) - (left < right)


def _compare_identifiers(left: str, right: str) -> int:
    left_numeric = _is_numeric(left)
    right_numeric = _is_numeric(right)
    if left_numeric and right_numeric:
        return _sign(len(left), len(right)) or _sign_text(left, right)
    if left_numeric:
        return -1
    if right_numeric:
        return 1
    return _sign_text(left, right)


def _sign_text(left: str, right: str) -> int:
    return (left > right) - (left < right)


def compare(left: Version, right: Version) -> int:
    for a, b in (
        (left.major, right.major),
        (left.minor, right.minor),
        (left.patch, right.patch),
    ):
        if a != b:
            return _sign(a, b)
    if not left.prerelease or not right.prerelease:
        return _sign(len(right.prerelease), len(left.prerelease))
    for a_id, b_id in zip(left.prerelease, right.prerelease, strict=False):
        order = _compare_identifiers(a_id, b_id)
        if order != 0:
            return order
    return _sign(len(left.prerelease), len(right.prerelease))
