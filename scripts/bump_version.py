#!/usr/bin/env python3
"""Increment the patch component in plugin-metadata.json.

The AMBuild metadata may contain a git placeholder after the semantic version,
for example ``1.2.3.{{git-shorthash}}``. The placeholder is preserved.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


VERSION_PATTERN = re.compile(r"^(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)(?P<suffix>.*)$")


def increment_patch(version: str) -> str:
    match = VERSION_PATTERN.fullmatch(version)
    if match is None:
        raise ValueError(
            f"version {version!r} must start with MAJOR.MINOR.PATCH"
        )

    patch = int(match.group("patch")) + 1
    return f"{match.group('major')}.{match.group('minor')}.{patch}{match.group('suffix')}"


def bump_metadata(path: Path) -> str:
    with path.open("r", encoding="utf-8") as metadata_file:
        metadata = json.load(metadata_file)

    old_version = metadata.get("version")
    if not isinstance(old_version, str):
        raise ValueError("plugin metadata must contain a string version")

    new_version = increment_patch(old_version)
    metadata["version"] = new_version
    with path.open("w", encoding="utf-8") as metadata_file:
        json.dump(metadata, metadata_file, indent=4)
        metadata_file.write("\n")

    return new_version


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--file",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "plugin-metadata.json",
        help="metadata file to update",
    )
    args = parser.parse_args()

    print(bump_metadata(args.file))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
