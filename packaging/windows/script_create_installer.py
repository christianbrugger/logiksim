"""Build LogikSim and create Windows installer."""

from __future__ import annotations

import argparse
import contextlib
import os
import shutil
import subprocess
from pathlib import Path

#
# FOLDERS
#

LS_ROOT = Path(__file__).parent.parent.parent.resolve()
LS_TEMP_PATH = Path(__file__).parent.resolve() / "temp"

LS_BUILD_PATH = LS_TEMP_PATH / "build"
LS_DEPLOY_PATH = LS_TEMP_PATH / "deploy"
LS_BUNDLE_PATH = LS_TEMP_PATH / "package"

#
# CONFIGURATION
#

LS_CMAKE_CONFIG = "win-clang-release"
LS_GUI_BINARY_SRC = "ls_gui.exe"
LS_GUI_BINARY_DST = "logiksim.exe"
LS_TEST_BINARIES = ["ls_test_core.exe", "ls_test_gui.exe"]
LS_RESOURCE_DIRS = ["resources/fonts", "resources/icons"]

# folder under $env:VCToolsRedistDir
LS_REDIST_FOLDER = "x64/Microsoft.VC143.CRT"

LS_ICON_SRC = LS_ROOT / "resources" / "icons" / "own" / "app_icon_256.ico"
LS_ICON_DST = LS_DEPLOY_PATH / "logiksim.ico"


def action_clean() -> None:
    """Delete all generated files and folders."""
    with contextlib.suppress(FileNotFoundError):
        shutil.rmtree(LS_TEMP_PATH)


def action_configure() -> None:
    """Configure cmake."""
    subprocess.run(
        ["cmake", "--preset", LS_CMAKE_CONFIG, "-B", LS_BUILD_PATH],
        check=True,
        cwd=LS_ROOT,
    )


def action_build() -> None:
    """Build the logiksim binary. Also run tests"""
    subprocess.run(["ninja"], check=True, cwd=LS_BUILD_PATH)

    for test_binary in LS_TEST_BINARIES:
        subprocess.run([LS_BUILD_PATH / test_binary], check=True, cwd=LS_BUILD_PATH)


def folder_only_contains_dll(folder: Path):
    assert folder.is_dir()

    for item in folder.iterdir():
        assert item.is_file()
        assert item.suffix == ".dll"


def action_deploy() -> None:
    """Collect qt libraries."""
    with contextlib.suppress(FileNotFoundError):
        shutil.rmtree(LS_DEPLOY_PATH)
    LS_DEPLOY_PATH.mkdir()

    # application files
    shutil.copy(LS_BUILD_PATH / LS_GUI_BINARY_SRC, LS_DEPLOY_PATH / LS_GUI_BINARY_DST)
    for resource_dir in LS_RESOURCE_DIRS:
        shutil.copytree(LS_BUILD_PATH / resource_dir, LS_DEPLOY_PATH / resource_dir)

    # qt libraries
    subprocess.run(
        [
            "windeployqt",
            "--release",
            #
            "--no-translations",
            "--no-compiler-runtime",
            #
            "--no-system-d3d-compiler",
            "--no-system-dxc-compiler",
            "--no-opengl-sw",
            #
            "--no-network",
            LS_GUI_BINARY_DST,
        ],
        check=True,
        cwd=LS_DEPLOY_PATH,
    )

    # vc redist
    redist_base = os.getenv("VCToolsRedistDir")
    assert redist_base is not None

    redist_dir = (Path(redist_base) / LS_REDIST_FOLDER).resolve()
    folder_only_contains_dll(redist_dir)

    for item in redist_dir.glob("*.dll"):
        shutil.copy(item, LS_DEPLOY_PATH / item.name)

    # icon
    shutil.copy(LS_ICON_SRC, LS_ICON_DST)


def action_package() -> None:
    """Package binary and ressources into windows installer."""


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "action",
        choices=["clean", "configure", "build", "deploy", "package"],
        help="Action to perform. Need to be run in order.",
    )
    args = parser.parse_args()

    if args.action == "clean":
        action_clean()
    elif args.action == "configure":
        action_configure()
    elif args.action == "build":
        action_build()
    elif args.action == "deploy":
        action_deploy()
    elif args.action == "package":
        action_package()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
