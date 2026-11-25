from __future__ import annotations

import sys
from collections.abc import Callable
from typing import TextIO

from ciutil import __version__
from ciutil.adler32 import adler32
from ciutil.b64 import decode, encode
from ciutil.crc32 import crc32
from ciutil.roman import from_roman, to_roman
from ciutil.semver import compare, parse

_ENCODING = "utf-8"

USAGE = """\
usage: ciutil <command> [args]
commands:
  crc32 STRING
  adler32 STRING
  b64enc STRING
  b64dec BASE64
  roman NUMBER
  unroman ROMAN
  semver-cmp A B
  version
"""

Handler = Callable[[list[str]], bytes]


def _utf8(text: str) -> bytes:
    return text.encode(_ENCODING, "surrogateescape")


def _line(text: str) -> bytes:
    return f"{text}\n".encode(_ENCODING)


def _parse_number(text: str) -> int:
    digits = text[1:] if text.startswith("-") else text
    if not (digits.isascii() and digits.isdigit()):
        raise ValueError(f"invalid number: {text!r}")
    if len(digits) > 18:
        raise ValueError(f"number out of range: {text!r}")
    return int(text)


def _cmd_crc32(params: list[str]) -> bytes:
    return _line(f"{crc32(_utf8(params[0])):08x}")


def _cmd_adler32(params: list[str]) -> bytes:
    return _line(f"{adler32(_utf8(params[0])):08x}")


def _cmd_b64enc(params: list[str]) -> bytes:
    return _line(encode(_utf8(params[0])))


def _cmd_b64dec(params: list[str]) -> bytes:
    return decode(params[0])


def _cmd_roman(params: list[str]) -> bytes:
    return _line(to_roman(_parse_number(params[0])))


def _cmd_unroman(params: list[str]) -> bytes:
    return _line(str(from_roman(params[0])))


def _cmd_semver_cmp(params: list[str]) -> bytes:
    left = parse(params[0])
    right = parse(params[1])
    return _line(str(compare(left, right)))


def _cmd_version(params: list[str]) -> bytes:
    return _line(f"ciutil python {__version__}")


_COMMANDS: dict[str, tuple[int, Handler]] = {
    "crc32": (1, _cmd_crc32),
    "adler32": (1, _cmd_adler32),
    "b64enc": (1, _cmd_b64enc),
    "b64dec": (1, _cmd_b64dec),
    "roman": (1, _cmd_roman),
    "unroman": (1, _cmd_unroman),
    "semver-cmp": (2, _cmd_semver_cmp),
    "version": (0, _cmd_version),
}


def _write(stream: TextIO, data: bytes) -> None:
    stream.flush()
    stream.buffer.write(data)
    stream.buffer.flush()


def _usage() -> int:
    _write(sys.stderr, USAGE.encode(_ENCODING))
    return 64


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if not args or args[0] not in _COMMANDS:
        return _usage()
    arity, handler = _COMMANDS[args[0]]
    params = args[1:]
    if len(params) != arity:
        return _usage()
    try:
        output = handler(params)
    except ValueError as error:
        _write(sys.stderr, f"ciutil: {error}\n".encode(_ENCODING, "replace"))
        return 2
    _write(sys.stdout, output)
    return 0
