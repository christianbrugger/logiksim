"""Copy the core library for logicsim to the output directory."""

import argparse
import shutil
from pathlib import Path

PROJECT = Path(__file__).parent.resolve()

SOURCE = {
    "debug": Path("../../build/win-clang-debug/src/core_export"),
    "release": Path("../../build/win-clang-release/src/core_export"),
}

DESTINATION = {
    "debug": Path("x64/Debug/main_winui3"),
    "release": Path("x64/Release/main_winui3"),
}

FILE_NAMES = {
    "debug": [
        "ls_core_shared.dll",
        "ls_core_shared.lib",
        "ls_core_shared.pdb",
    ],
    "release": [
        "ls_core_shared.dll",
        "ls_core_shared.lib",
    ],
}


def main() -> None:
    """Parse args & copy files."""
    parser = argparse.ArgumentParser()
    parser.add_argument("configuration", choices=["debug", "release"])
    args = parser.parse_args()
    config = args.configuration

    for file in FILE_NAMES[config]:
        print("Copy file:", file)
        shutil.copy2(SOURCE[config] / file, DESTINATION[config] / file)


if __name__ == "__main__":
    raise SystemExit(main())
