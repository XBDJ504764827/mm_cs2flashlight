#!/usr/bin/env python3

import json
import tempfile
import unittest
from pathlib import Path

from release_version import resolve_release_version, write_metadata_version


class ReleaseVersionTests(unittest.TestCase):
    def test_first_release_uses_metadata_version(self):
        self.assertEqual(
            resolve_release_version("1.0.0.{{git-shorthash}}", [], []),
            "1.0.0",
        )

    def test_existing_release_increments_patch(self):
        self.assertEqual(
            resolve_release_version(
                "1.0.0.{{git-shorthash}}", ["v1.0.0", "invalid"], []
            ),
            "1.0.1",
        )

    def test_metadata_can_raise_minor_version(self):
        self.assertEqual(
            resolve_release_version(
                "1.2.0.{{git-shorthash}}", ["v1.0.9", "v1.1.3"], []
            ),
            "1.2.0",
        )

    def test_rerun_reuses_tag_on_head(self):
        self.assertEqual(
            resolve_release_version(
                "1.0.1.{{git-shorthash}}", ["v1.0.1"], ["v1.0.1"]
            ),
            "1.0.1",
        )

    def test_writes_build_version_with_git_suffix(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "plugin-metadata.json"
            path.write_text(
                json.dumps({"name": "test", "version": "1.0.0.old"}),
                encoding="utf-8",
            )

            write_metadata_version(path, "2.3.4")

            metadata = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(metadata["version"], "2.3.4.{{git-shorthash}}")


if __name__ == "__main__":
    unittest.main()
