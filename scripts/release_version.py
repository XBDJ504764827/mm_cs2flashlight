#!/usr/bin/env python3
"""Resolve and optionally write the version for a GitHub Release build."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path
from typing import Iterable


SEMVER_PREFIX = re.compile(
    r"^v?(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)(?P<suffix>.*)$"
)
RELEASE_TAG = re.compile(r"^v(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)$")
GIT_VERSION_SUFFIX = ".{{git-shorthash}}"


def parse_version(value: str) -> tuple[int, int, int]:
    match = SEMVER_PREFIX.fullmatch(value)
    if match is None:
        raise ValueError(f"version {value!r} must start with MAJOR.MINOR.PATCH")
    return tuple(int(match.group(part)) for part in ("major", "minor", "patch"))


def parse_release_tags(tags: Iterable[str]) -> list[tuple[int, int, int]]:
    versions = []
    for tag in tags:
        match = RELEASE_TAG.fullmatch(tag.strip())
        if match is not None:
            versions.append(
                tuple(int(match.group(part)) for part in ("major", "minor", "patch"))
            )
    return versions


def format_version(version: tuple[int, int, int]) -> str:
    return ".".join(str(part) for part in version)


def resolve_release_version(
    metadata_version: str,
    all_tags: Iterable[str],
    head_tags: Iterable[str],
) -> str:
    head_versions = parse_release_tags(head_tags)
    if head_versions:
        # A rerun of the same merge must update its existing Release, not create another one.
        return format_version(max(head_versions))

    metadata = parse_version(metadata_version)
    released_versions = parse_release_tags(all_tags)
    if not released_versions:
        return format_version(metadata)

    latest = max(released_versions)
    next_patch = (latest[0], latest[1], latest[2] + 1)
    return format_version(max(metadata, next_patch))


def read_metadata_version(path: Path) -> str:
    with path.open("r", encoding="utf-8") as metadata_file:
        metadata = json.load(metadata_file)

    version = metadata.get("version")
    if not isinstance(version, str):
        raise ValueError("plugin metadata must contain a string version")
    return version


def write_metadata_version(path: Path, version: str) -> None:
    parsed = format_version(parse_version(version))
    with path.open("r", encoding="utf-8") as metadata_file:
        metadata = json.load(metadata_file)

    metadata["version"] = parsed + GIT_VERSION_SUFFIX
    with path.open("w", encoding="utf-8") as metadata_file:
        json.dump(metadata, metadata_file, indent=4)
        metadata_file.write("\n")


def git_tags(*arguments: str) -> list[str]:
    result = subprocess.run(
        ["git", "tag", *arguments],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.splitlines()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--metadata",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "plugin-metadata.json",
    )
    parser.add_argument(
        "--current",
        action="store_true",
        help="Print the metadata version without calculating the next release.",
    )
    parser.add_argument(
        "--set-version",
        help="Use an explicit version, primarily to synchronize main after publishing.",
    )
    parser.add_argument(
        "--write",
        action="store_true",
        help="Write the resolved version back to plugin-metadata.json.",
    )
    args = parser.parse_args()

    metadata_version = read_metadata_version(args.metadata)
    if args.set_version:
        version = format_version(parse_version(args.set_version))
    elif args.current:
        version = format_version(parse_version(metadata_version))
    else:
        version = resolve_release_version(
            metadata_version,
            git_tags("--list", "v*"),
            git_tags("--points-at", "HEAD", "--list", "v*"),
        )

    if args.write:
        write_metadata_version(args.metadata, version)

    print(version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
