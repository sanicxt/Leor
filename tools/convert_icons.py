#!/usr/bin/env python3
"""
Convert monochrome SVG icons to C header files for u8g2 DrawXBMP.

Pipeline:
    SVG → rsvg-convert → PNG → composite over white → grayscale → invert
    → 1-bit threshold (dither=off) → pack bits MSB-first → inline constexpr C array

Usage:
    python3 tools/convert_icons.py --input assets/icons --output components/leor_core/include/leor/icons
"""

import argparse
import io
import os
import re
import subprocess
import sys
from pathlib import Path

from PIL import Image


def svg_to_bitmap(svg_path: Path, size: int = 16) -> bytes:
    """Render SVG to packed 1-bit bitmap bytes (MSB=leftmost)."""
    result = subprocess.run(
        [
            "rsvg-convert",
            "--width", str(size),
            "--height", str(size),
            "--format", "png",
            str(svg_path),
        ],
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"rsvg-convert failed on {svg_path}: {result.stderr.decode()}"
        )

    overlay = Image.open(io.BytesIO(result.stdout)).convert("RGBA")
    bg = Image.new("RGBA", (size, size), (255, 255, 255, 255))
    composite = Image.alpha_composite(bg, overlay)
    gray = composite.convert("L")

    # Invert: dark paths become white (on), white bg becomes black (off)
    inverted = Image.eval(gray, lambda x: 255 - x)

    # Threshold at 128, no dither. 1 = white = ON = drawn pixel
    bw = inverted.convert("1", dither=Image.NONE)

    pixels = list(bw.get_flattened_data())
    bytes_per_row = (size + 7) // 8
    out = bytearray()
    for row in range(size):
        row_pixels = pixels[row * size : (row + 1) * size]
        for col_byte in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                idx = col_byte * 8 + bit
                if idx < size and row_pixels[idx]:
                    byte |= 1 << bit
            out.append(byte)
    return bytes(out)


def icon_name(svg_path: Path) -> str:
    """Derive C identifier from SVG filename.

    icons8-phone.svg       → phone
    icons8-facebook-messenger.svg → facebook_messenger
    """
    name = svg_path.stem
    name = re.sub(r"^icons8-", "", name, flags=re.IGNORECASE)
    name = name.replace("-", "_").replace(".", "_")
    return name.lower()


def generate_header(name: str, bitmap: bytes, size: int = 16) -> str:
    """Build C header file content for one icon."""
    lines = []
    for i in range(0, len(bitmap), 16):
        chunk = bitmap[i : i + 16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk))

    body = ",\n".join(lines)

    # Bitmap format: packed bits MSB-first, bytes_per_row = ceil(w/8).
    # bit=1 = pixel ON (white on OLED). Total: rows * bytes_per_row bytes.
    return f"""\
#pragma once
#include <cstdint>

namespace leor::icons {{
inline constexpr int {name}_w = {size};
inline constexpr int {name}_h = {size};
inline constexpr uint8_t {name}[] = {{
{body}
}};
}}  // namespace leor::icons
"""


def generate_master(names: list[str]) -> str:
    includes = "\n".join(f'#include "icon_{n}.h"' for n in names)
    return f"#pragma once\n{includes}\n"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert SVG icons to C headers for u8g2 DrawXBMP"
    )
    parser.add_argument("--input", required=True, help="Input directory with *.svg")
    parser.add_argument("--output", required=True, help="Output directory for .h files")
    parser.add_argument(
        "--size", type=int, default=16, help="Icon size in pixels (default: 16)"
    )
    args = parser.parse_args()

    input_dir = Path(args.input)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    svgs = sorted(input_dir.glob("*.svg"))
    if not svgs:
        print("No SVG files found.")
        sys.exit(0)

    generated: list[str] = []
    for svg in svgs:
        name = icon_name(svg)
        bitmap = svg_to_bitmap(svg, args.size)
        content = generate_header(name, bitmap, args.size)

        out = output_dir / f"icon_{name}.h"
        out.write_text(content)
        generated.append(name)
        print(f"  {svg.name}  →  {out.name}  ({len(bitmap)} B)")

    (output_dir / "icons.h").write_text(generate_master(generated))
    print(f"\nWrote {len(generated)} icon headers + icons.h")

    # Prune stale headers (SVG was removed)
    kept = {f"icon_{n}.h" for n in generated}
    kept.add("icons.h")
    for f in output_dir.glob("*.h"):
        if f.name not in kept:
            f.unlink()
            print(f"  Pruned stale: {f.name}")


if __name__ == "__main__":
    main()
