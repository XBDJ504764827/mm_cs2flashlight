#!/usr/bin/env python3

import unittest

from bump_version import increment_patch


class IncrementPatchTests(unittest.TestCase):
    def test_preserves_git_placeholder(self):
        self.assertEqual(
            increment_patch("1.0.9.{{git-shorthash}}"),
            "1.0.10.{{git-shorthash}}",
        )

    def test_preserves_prerelease_suffix(self):
        self.assertEqual(increment_patch("2.4.1-beta.2"), "2.4.2-beta.2")

    def test_rejects_invalid_version(self):
        with self.assertRaises(ValueError):
            increment_patch("development")


if __name__ == "__main__":
    unittest.main()
