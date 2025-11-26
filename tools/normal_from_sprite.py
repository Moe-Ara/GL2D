"""
Generate a tangent-space normal map from a sprite image by deriving height from luminance.

Usage:
  python tools/normal_from_sprite.py input.png output.png --strength 2.0 --blur 1.0

Requires: Pillow (pip install pillow)
"""

import argparse
import math
import sys
from pathlib import Path

try:
    from PIL import Image, ImageFilter
except ImportError as exc:  # pragma: no cover
    print("Pillow is required: pip install pillow", file=sys.stderr)
    raise SystemExit(1) from exc


def sobel_normal_map(img: Image.Image, strength: float, blur: float) -> Image.Image:
    rgba = img.convert("RGBA")
    height = rgba.convert("L")
    if blur > 0.0:
        height = height.filter(ImageFilter.GaussianBlur(radius=blur))

    w, h = height.size
    src = height.load()
    alpha_src = rgba.split()[-1].load()

    def sample(x: int, y: int) -> float:
        x = 0 if x < 0 else w - 1 if x >= w else x
        y = 0 if y < 0 else h - 1 if y >= h else y
        return src[x, y] / 255.0

    out = Image.new("RGBA", (w, h))
    dst = out.load()

    # Sobel kernels
    kx = ((-1, 0, 1), (-2, 0, 2), (-1, 0, 1))
    ky = ((-1, -2, -1), (0, 0, 0), (1, 2, 1))

    for y in range(h):
        for x in range(w):
            gx = 0.0
            gy = 0.0
            for j in range(3):
                for i in range(3):
                    v = sample(x + i - 1, y + j - 1)
                    gx += kx[j][i] * v
                    gy += ky[j][i] * v

            nx = -gx * strength
            ny = -gy * strength
            nz = 1.0
            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            if length > 0.0001:
                nx /= length
                ny /= length
                nz /= length
            r = int((nx * 0.5 + 0.5) * 255.0 + 0.5)
            g = int((ny * 0.5 + 0.5) * 255.0 + 0.5)
            b = int((nz * 0.5 + 0.5) * 255.0 + 0.5)
            a = alpha_src[x, y]
            dst[x, y] = (r, g, b, a)

    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate a normal map from a sprite image.")
    parser.add_argument("input", type=Path, help="Input sprite path or directory (e.g., assets/character)")
    parser.add_argument("output", nargs="?", type=Path,
                        help="Output file or directory. If input is a directory, output must be a directory.")
    parser.add_argument("--strength", type=float, default=2.0, help="Height-to-normal strength (default: 2.0)")
    parser.add_argument("--blur", type=float, default=0.0, help="Gaussian blur radius before normalizing (default: 0.0)")
    args = parser.parse_args()

    if not args.input.exists():
        print(f"Input does not exist: {args.input}", file=sys.stderr)
        return 1

    def process_file(src: Path, dst: Path) -> None:
        img = Image.open(src)
        normal = sobel_normal_map(img, strength=args.strength, blur=args.blur)
        dst.parent.mkdir(parents=True, exist_ok=True)
        normal.save(dst)
        print(f"Wrote normal map to {dst}")

    if args.input.is_file():
        if args.output is None:
            out = args.input.with_stem(args.input.stem + "_normal")
        else:
            out = args.output
        process_file(args.input, out)
        return 0

    # Directory mode: create a "normal maps" subfolder and mirror filenames.
    src_dir = args.input
    out_dir = args.output if args.output is not None else src_dir / "normal maps"
    if not out_dir.exists():
        out_dir.mkdir(parents=True, exist_ok=True)

    for path in sorted(src_dir.rglob("*.png")):
        rel = path.relative_to(src_dir)
        dst = out_dir / rel.parent / path.name
        process_file(path, dst)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
