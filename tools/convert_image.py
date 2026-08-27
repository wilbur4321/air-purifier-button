"""
Converts an image into a RGB565 C array for embedding in flash and drawing
with TFT_eSPI's pushImage(). Fits the source image into the target
resolution (letterboxed on black, aspect preserved).

Usage:
    python tools/convert_image.py assets/off.png include/off_image.h off_image 320 240
"""

import sys
from pathlib import Path

from PIL import Image


def convert(src_path, dst_path, array_name, width, height):
    img = Image.open(src_path).convert("RGB")

    scale = min(width / img.width, height / img.height)
    new_size = (max(1, round(img.width * scale)), max(1, round(img.height * scale)))
    resized = img.resize(new_size, Image.LANCZOS)

    canvas = Image.new("RGB", (width, height), (0, 0, 0))
    offset = ((width - new_size[0]) // 2, (height - new_size[1]) // 2)
    canvas.paste(resized, offset)

    pixels = list(canvas.getdata())
    rgb565 = []
    for r, g, b in pixels:
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        rgb565.append(value)

    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        f"constexpr int {array_name}_width = {width};",
        f"constexpr int {array_name}_height = {height};",
        f"const uint16_t {array_name}[{width * height}] = {{",
    ]
    for i in range(0, len(rgb565), 16):
        chunk = rgb565[i : i + 16]
        lines.append("    " + ", ".join(f"0x{v:04X}" for v in chunk) + ",")
    lines.append("};")
    lines.append("")

    Path(dst_path).write_text("\n".join(lines))
    print(f"Wrote {dst_path} ({width}x{height}, {len(rgb565) * 2} bytes)")


if __name__ == "__main__":
    if len(sys.argv) != 6:
        print(__doc__)
        sys.exit(1)

    src, dst, name, w, h = sys.argv[1:]
    convert(src, dst, name, int(w), int(h))
