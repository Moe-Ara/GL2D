#!/usr/bin/env python
"""
Generate per-character animation metadata (animations.json) by inspecting sprite sheets.

The input config describes each character, the animations they offer, and optional grid hints.
The script derives UV rectangles from uniform grids and will append a normalTexture path if one isn't
explicitly specified (it appends the configured suffix to the diffuse texture).

Usage:
  python tools/generate_animation_metadata.py --config tools/animation_profiles.json

The config format:
{
  "normalSuffix": "_normal",
  "characters": [
    {
      "name": "Enchantress",
      "directory": "Demos/The Lost Heroin/assets/women/Enchantress",
      "output": "Demos/The Lost Heroin/assets/women/Enchantress/animations.json",
      "initialState": "Rest",
      "defaultFrameDuration": 0.08,
      "animations": [
        {
          "name": "Rest",
          "texture": "Rest.png",
          "rows": 1,
          "cols": 3
        },
        ...
      ]
    }
  ]
}
"""

import argparse
import json
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from PIL import Image


def _relative_path(path: Path, base_path: Path) -> str:
    try:
        return str(path.relative_to(base_path))
    except ValueError:
        return str(path)


def guess_grid(image: Image.Image, rows: Optional[int], cols: Optional[int]) -> Tuple[int, int]:
    width, height = image.size
    if rows and cols:
        return rows, cols
    if rows:
        frame_height = height // rows
        if frame_height > 0 and width % frame_height == 0:
            return rows, width // frame_height
    if cols:
        frame_width = width // cols
        if frame_width > 0 and height % frame_width == 0:
            return height // frame_width, cols
    if width % height == 0:
        return 1, width // height
    if height % width == 0:
        return height // width, 1
    for candidate_rows in range(1, min(16, height) + 1):
        if height % candidate_rows != 0:
            continue
        frame_height = height // candidate_rows
        if frame_height == 0:
            continue
        if width % frame_height == 0:
            return candidate_rows, width // frame_height
    for candidate_cols in range(1, min(16, width) + 1):
        if width % candidate_cols != 0:
            continue
        frame_width = width // candidate_cols
        if frame_width == 0:
            continue
        if height % frame_width == 0:
            return height // frame_width, candidate_cols
    return 1, 1


def build_frames(rows: int,
                 cols: int,
                 texture: str,
                 normal_texture: Optional[str],
                 frame_limit: Optional[int],
                 atlas_rows_hint: Optional[int]) -> List[Dict]:
    frames = []
    max_frames = rows * cols if frame_limit is None else min(frame_limit, rows * cols)
    for row in range(rows):
        if len(frames) >= max_frames:
            break
        for col in range(cols):
            if len(frames) >= max_frames:
                break
            u0 = col / cols
            v0 = row / rows
            u1 = (col + 1) / cols
            v1 = (row + 1) / rows
            frame = {
                "uv": [u0, v0, u1, v1],
                "texture": texture,
            }
            if normal_texture:
                frame["normalTexture"] = normal_texture
            frames.append(frame)
    return frames


def _resolve_normal_texture(base_path: Path,
                            texture_path: str,
                            normal_suffix: str,
                            anim_normal_dir: Optional[str],
                            default_normal_dir: Optional[str]) -> str:
    normal_name = f"{Path(texture_path).stem}{normal_suffix}{Path(texture_path).suffix}"
    dir_source = anim_normal_dir or default_normal_dir
    if dir_source:
        normal_dir = Path(dir_source)
        if not normal_dir.is_absolute():
            normal_dir = base_path / normal_dir
    else:
        normal_dir = base_path
    normal_abs = normal_dir / normal_name
    return _relative_path(normal_abs, base_path)


def generate_animation_entry(anim: Dict,
                             base_path: Path,
                             normal_suffix: str,
                             default_normal_dir: Optional[str]) -> Dict:
    texture_path = anim["texture"]
    texture_full = base_path / texture_path
    if not texture_full.exists():
        raise FileNotFoundError(f"{texture_full} does not exist")
    image = Image.open(texture_full)
    rows, cols = guess_grid(image, anim.get("rows"), anim.get("cols"))
    normal_texture = anim.get("normalTexture")
    if not normal_texture:
        normal_texture = _resolve_normal_texture(base_path,
                                                 texture_path,
                                                 normal_suffix,
                                                 anim.get("normalDirectory"),
                                                 default_normal_dir)
    else:
        normal_full = Path(normal_texture)
        if not normal_full.is_absolute():
            normal_full = base_path / normal_full
        normal_texture = _relative_path(normal_full, base_path)
    frames = build_frames(rows,
                          cols,
                          texture_path,
                          normal_texture,
                          anim.get("frameCount"),
                          anim.get("rows"))
    return {
        "name": anim["name"],
        "loop": anim.get("loop", True),
        "playback": anim.get("playback", "forward"),
        "frameDuration": anim.get("frameDuration", None),
        "frames": frames
    }


def generate_metadata(profile: Dict, normal_suffix: str) -> Dict:
    anims = []
    base_path = Path(profile["directory"])
    default_normal_dir = profile.get("normalDirectory")
    for anim in profile["animations"]:
        anims.append(generate_animation_entry(anim, base_path, normal_suffix, default_normal_dir))
    result = {
        "initialState": profile.get("initialState", anims[0]["name"] if anims else ""),
        "defaultFrameDuration": profile.get("defaultFrameDuration", 0.1),
        "atlas": profile.get("atlas", {"texture": "", "rows": 1, "cols": 1}),
        "animations": anims
    }
    return result


def generate_from_source(source: Path,
                         normal_suffix: str,
                         rows: Optional[int],
                         cols: Optional[int],
                         animation_name: Optional[str],
                         output: Optional[Path]) -> None:
    if not source.exists():
        raise FileNotFoundError(f"Source path does not exist: {source}")
    textures = []
    if source.is_file():
        textures = [source]
    else:
        textures = sorted(
            x for x in source.iterdir()
            if x.is_file() and x.suffix.lower() in (".png", ".jpg", ".jpeg"))
    if not textures:
        raise ValueError("No sprite textures found in the provided source")

    anims = []
    for tex in textures:
        name = animation_name or tex.stem
        entry = {
            "name": name,
            "loop": True,
            "playback": "forward",
            "texture": tex.name,
        }
        if rows:
            entry["rows"] = rows
        if cols:
            entry["cols"] = cols
        anims.append(entry)

    metadata = {
        "initialState": anims[0]["name"],
        "defaultFrameDuration": 0.1,
        "atlas": {"texture": "", "rows": 1, "cols": 1},
        "animations": [generate_animation_entry(anim, source.parent if source.is_file() else source, normal_suffix)
                       for anim in anims]
    }

    out_path = output
    if out_path is None:
        out_path = source.parent / "animations.json" if source.is_file() else source / "animations.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as fh:
        json.dump(metadata, fh, indent=2)
    print(f"Generated {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate animation metadata JSON.")
    parser.add_argument("--config",
                        type=Path,
                        default=Path("tools/animation_profiles.json"),
                        help="Path to the animation profile configuration.")
    parser.add_argument("--source",
                        type=Path,
                        help="Single sprite or directory of sprites to turn into metadata.")
    parser.add_argument("--output", type=Path,
                        help="Override the output metadata path when using --source.")
    parser.add_argument("--rows", type=int, help="Grid rows when automatically generating metadata.")
    parser.add_argument("--cols", type=int, help="Grid cols when automatically generating metadata.")
    parser.add_argument("--animation-name", type=str,
                        help="Name to use for all animations when generating from a single file.")
    args = parser.parse_args()

    normal_suffix = "_normal"
    if args.source:
        generate_from_source(args.source, normal_suffix, args.rows, args.cols, args.animation_name, args.output)
        return

    if not args.config.exists():
        raise FileNotFoundError(f"Config not found: {args.config}")

    with args.config.open() as fh:
        config = json.load(fh)
    normal_suffix = config.get("normalSuffix", normal_suffix)

    for character in config.get("characters", []):
        output_path = Path(character["output"])
        metadata = generate_metadata(character, normal_suffix)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with output_path.open("w", encoding="utf-8") as fh:
            json.dump(metadata, fh, indent=2)
        print(f"Generated {output_path}")


if __name__ == "__main__":
    main()
