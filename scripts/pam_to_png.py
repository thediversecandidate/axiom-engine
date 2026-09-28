#!/usr/bin/env python3
"""Convert RGBA PAM images (as written by the renderer tests) to PNG, standard library only.

usage: pam_to_png.py <file.pam | directory>...
Each input.pam becomes input.png next to it. Used by CI before uploading test images, and for
reviewing a candidate reference image (tests/golden/README.md).

Exit codes: 0 converted (or nothing to convert), 1 a file was not an RGBA PAM with MAXVAL 255.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path


def read_pam(data: bytes) -> tuple[int, int, bytes]:
    header_end = data.index(b"ENDHDR\n") + len(b"ENDHDR\n")
    lines = data[:header_end].decode("ascii").split("\n")
    if lines[0] != "P7":
        raise ValueError("not a PAM (P7) file")
    fields = dict(line.split(" ", 1) for line in lines[1:] if " " in line)
    width, height = int(fields["WIDTH"]), int(fields["HEIGHT"])
    if fields.get("DEPTH") != "4" or fields.get("MAXVAL") != "255" or fields.get("TUPLTYPE") != "RGB_ALPHA":
        raise ValueError("expected DEPTH 4, MAXVAL 255, TUPLTYPE RGB_ALPHA")
    payload = data[header_end:]
    if len(payload) != width * height * 4:
        raise ValueError(f"payload is {len(payload)} bytes, expected {width * height * 4}")
    return width, height, payload


def png_bytes(width: int, height: int, rgba: bytes) -> bytes:
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    stride = width * 4
    raw = b"".join(b"\x00" + rgba[y * stride:(y + 1) * stride] for y in range(height))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def convert(path: Path) -> Path:
    width, height, rgba = read_pam(path.read_bytes())
    out = path.with_suffix(".png")
    out.write_bytes(png_bytes(width, height, rgba))
    return out


def main(argv: list[str]) -> int:
    files: list[Path] = []
    for arg in argv:
        p = Path(arg)
        files.extend(sorted(p.rglob("*.pam")) if p.is_dir() else [p])
    status = 0
    for f in files:
        try:
            print(f"pam_to_png: {convert(f)}")
        except (ValueError, KeyError, OSError) as e:
            print(f"pam_to_png: {f}: {e}")
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
