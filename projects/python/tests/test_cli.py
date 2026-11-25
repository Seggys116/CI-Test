from __future__ import annotations

import subprocess
import sys
import zlib

import pytest

from ciutil import __version__
from ciutil.cli import main

OK_CASES: list[tuple[list[str], bytes]] = [
    (["version"], b"ciutil python 1.0.0\n"),
    (["crc32", "123456789"], b"cbf43926\n"),
    (["crc32", ""], b"00000000\n"),
    (["crc32", "a"], b"e8b7be43\n"),
    (["crc32", "The quick brown fox jumps over the lazy dog"], b"414fa339\n"),
    (["adler32", "Wikipedia"], b"11e60398\n"),
    (["adler32", ""], b"00000001\n"),
    (["adler32", "123456789"], b"091e01de\n"),
    (["b64enc", "foobar"], b"Zm9vYmFy\n"),
    (["b64enc", "foob"], b"Zm9vYg==\n"),
    (["b64enc", ""], b"\n"),
    (["b64enc", "é"], b"w6k=\n"),
    (["b64dec", "Zm9vYmFy"], b"foobar"),
    (["b64dec", "AP/+"], b"\x00\xff\xfe"),
    (["b64dec", "/w=="], b"\xff"),
    (["b64dec", "w6k="], b"\xc3\xa9"),
    (["b64dec", ""], b""),
    (["roman", "1994"], b"MCMXCIV\n"),
    (["roman", "3999"], b"MMMCMXCIX\n"),
    (["roman", "1"], b"I\n"),
    (["unroman", "MCMXCIV"], b"1994\n"),
    (["unroman", "MMMCMXCIX"], b"3999\n"),
    (["semver-cmp", "1.0.0-beta.2", "1.0.0-beta.11"], b"-1\n"),
    (["semver-cmp", "1.0.0+a", "1.0.0+b"], b"0\n"),
    (["semver-cmp", "2.0.0", "1.0.0"], b"1\n"),
    (["semver-cmp", "1.0.0-rc.1", "1.0.0"], b"-1\n"),
]

ERROR_CASES: list[list[str]] = [
    ["b64dec", "Zh=="],
    ["b64dec", "Zg="],
    ["b64dec", "AP_-"],
    ["b64dec", "Zg==Zg=="],
    ["b64dec", "Zm9v\n"],
    ["roman", "0"],
    ["roman", "4000"],
    ["roman", "-1"],
    ["roman", "abc"],
    ["roman", ""],
    ["roman", "1.5"],
    ["roman", "99999999999999999999999999"],
    ["unroman", "IIII"],
    ["unroman", "xiv"],
    ["unroman", ""],
    ["unroman", "VX"],
    ["semver-cmp", "01.0.0", "1.0.0"],
    ["semver-cmp", "1.0", "1.0.0"],
    ["semver-cmp", "1.0.0", "v1.0.0"],
]

USAGE_CASES: list[list[str]] = [
    [],
    ["bogus"],
    ["crc32"],
    ["crc32", "a", "b"],
    ["adler32"],
    ["b64enc"],
    ["b64dec"],
    ["roman"],
    ["unroman", "I", "II"],
    ["semver-cmp"],
    ["semver-cmp", "1.0.0"],
    ["semver-cmp", "1.0.0", "1.0.0", "1.0.0"],
    ["version", "extra"],
    ["--help"],
]


def run_cli(*args: str) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        [sys.executable, "-m", "ciutil", *args],
        capture_output=True,
        check=False,
    )


@pytest.mark.parametrize(("args", "expected"), OK_CASES)
def test_subprocess_success(args: list[str], expected: bytes) -> None:
    result = run_cli(*args)
    assert result.returncode == 0
    assert result.stdout == expected
    assert result.stderr == b""


@pytest.mark.parametrize("args", ERROR_CASES)
def test_subprocess_error_exit_code_two(args: list[str]) -> None:
    result = run_cli(*args)
    assert result.returncode == 2
    assert result.stdout == b""
    assert result.stderr.endswith(b"\n")
    assert len(result.stderr) > 1


@pytest.mark.parametrize("args", USAGE_CASES)
def test_subprocess_usage_exit_code_64(args: list[str]) -> None:
    result = run_cli(*args)
    assert result.returncode == 64
    assert result.stdout == b""
    assert result.stderr.startswith(b"usage: ciutil")


@pytest.mark.parametrize(("args", "expected"), OK_CASES)
def test_main_success(
    args: list[str], expected: bytes, capsysbinary: pytest.CaptureFixture[bytes]
) -> None:
    assert main(args) == 0
    captured = capsysbinary.readouterr()
    assert captured.out == expected
    assert captured.err == b""


@pytest.mark.parametrize("args", ERROR_CASES)
def test_main_error_exit_code_two(
    args: list[str], capsysbinary: pytest.CaptureFixture[bytes]
) -> None:
    assert main(args) == 2
    captured = capsysbinary.readouterr()
    assert captured.out == b""
    assert captured.err.startswith(b"ciutil: ")


@pytest.mark.parametrize("args", USAGE_CASES)
def test_main_usage_exit_code_64(
    args: list[str], capsysbinary: pytest.CaptureFixture[bytes]
) -> None:
    assert main(args) == 64
    captured = capsysbinary.readouterr()
    assert captured.out == b""
    assert captured.err.startswith(b"usage: ciutil")


def test_non_ascii_argument_is_hashed_as_utf8(
    capsysbinary: pytest.CaptureFixture[bytes],
) -> None:
    word = "héllo"
    assert main(["crc32", word]) == 0
    expected = f"{zlib.crc32(word.encode()):08x}\n".encode("ascii")
    assert capsysbinary.readouterr().out == expected


def test_output_uses_unix_newlines(capsysbinary: pytest.CaptureFixture[bytes]) -> None:
    assert main(["roman", "14"]) == 0
    assert capsysbinary.readouterr().out == b"XIV\n"


def test_version_matches_package(capsysbinary: pytest.CaptureFixture[bytes]) -> None:
    assert main(["version"]) == 0
    assert capsysbinary.readouterr().out == f"ciutil python {__version__}\n".encode()


def test_entry_point_reads_sys_argv(
    monkeypatch: pytest.MonkeyPatch, capsysbinary: pytest.CaptureFixture[bytes]
) -> None:
    monkeypatch.setattr(sys, "argv", ["ciutil", "crc32", "123456789"])
    assert main() == 0
    assert capsysbinary.readouterr().out == b"cbf43926\n"
