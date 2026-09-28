import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import pam_to_png  # noqa: E402

PIXELS = bytes([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 128, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])


def pam(width: int, height: int, payload: bytes, tupltype: str = "RGB_ALPHA") -> bytes:
    header = f"P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE {tupltype}\nENDHDR\n"
    return header.encode() + payload


def decode_png(data: bytes) -> tuple[int, int, bytes]:
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, width = 8, b"", 0
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        kind, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + length]
        assert struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])[0] == zlib.crc32(kind + body)
        if kind == b"IHDR":
            width, height = struct.unpack(">II", body[:8])
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    raw = zlib.decompress(idat)
    stride = width * 4
    rows = [raw[y * (stride + 1):(y + 1) * (stride + 1)] for y in range(height)]
    assert all(r[0] == 0 for r in rows)  # filter type 0
    return width, height, b"".join(r[1:] for r in rows)


def test_round_trip_preserves_pixels(tmp_path):
    src = tmp_path / "img.pam"
    src.write_bytes(pam(3, 2, PIXELS))
    assert pam_to_png.main([str(tmp_path)]) == 0
    assert decode_png((tmp_path / "img.png").read_bytes()) == (3, 2, PIXELS)


def test_truncated_and_wrong_type_are_rejected(tmp_path):
    (tmp_path / "short.pam").write_bytes(pam(3, 2, PIXELS[:-1]))
    (tmp_path / "gray.pam").write_bytes(pam(3, 2, PIXELS, "GRAYSCALE"))
    assert pam_to_png.main([str(tmp_path / "short.pam")]) == 1
    assert pam_to_png.main([str(tmp_path / "gray.pam")]) == 1
    assert not (tmp_path / "short.png").exists()
