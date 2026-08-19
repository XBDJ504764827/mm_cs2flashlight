#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


class GameDataTests(unittest.TestCase):
    def test_required_platform_values_are_present(self):
        path = Path(__file__).parents[1] / "gamedata" / "cs2_flashlight.games.txt"
        sections = {}
        current = None

        for raw_line in path.read_text(encoding="ascii").splitlines():
            line = raw_line.split("#", 1)[0].split(";", 1)[0].strip()
            if not line:
                continue
            if line.startswith("[") and line.endswith("]"):
                current = line[1:-1].strip()
                sections[current] = {}
                continue
            key, value = (part.strip() for part in line.split("=", 1))
            sections[current][key] = value

        required = {
            "game_entity_system_offset",
            "teleport_virtual_index",
            "create_entity_by_name",
            "dispatch_spawn",
            "accept_input",
        }
        pattern = re.compile(r"^(?:[0-9A-Fa-f]{2}|\?)(?: (?:[0-9A-Fa-f]{2}|\?))*$")

        for platform in ("linux", "windows"):
            self.assertEqual(set(sections[platform]), required)
            numeric = {"game_entity_system_offset", "teleport_virtual_index"}
            for key in numeric:
                self.assertGreaterEqual(int(sections[platform][key]), 0)
            for key in required - numeric:
                self.assertRegex(sections[platform][key], pattern)


if __name__ == "__main__":
    unittest.main()
