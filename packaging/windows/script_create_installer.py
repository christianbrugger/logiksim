"""Build LogikSim and create Windows installer."""

from __future__ import annotations

from pathlib import Path
import shutil

import argparse
import subprocess
import contextlib

LS_CMAKE_CONFIG = "win-clang-release"
LS_GUI_BINARY_SRC = "ls_gui.exe"
LS_GUI_BINARY_DST = "logiksim.exe"
LS_TEST_BINARIES = ["ls_test_core.exe", "ls_test_gui.exe"]
LS_RESOURCE_DIRS = ["resources/fonts", "resources/icons"]

LS_ROOT = Path(__file__).parent.parent.parent.resolve()
LS_TEMP_PATH = Path(__file__).parent.resolve() / "temp"

LS_BUILD_PATH = LS_TEMP_PATH / "build"
LS_DEPLOY_PATH = LS_TEMP_PATH / "deploy"
LS_BUNDLE_PATH = LS_TEMP_PATH / "package"


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


def action_deploy() -> None:
    """Collect qt libraries."""
    with contextlib.suppress(FileNotFoundError):
        shutil.rmtree(LS_DEPLOY_PATH)
    LS_DEPLOY_PATH.mkdir()

    shutil.copy(LS_BUILD_PATH / LS_GUI_BINARY_SRC, LS_DEPLOY_PATH / LS_GUI_BINARY_DST)
    for resource_dir in LS_RESOURCE_DIRS:
        shutil.copytree(LS_BUILD_PATH / resource_dir, LS_DEPLOY_PATH / resource_dir)

    subprocess.run(
        # ["windeployqt" "--release" "--no-translations", LS_GUI_BINARY_DST],
        ["cmake", "-h"],
        check=True,
        cwd=LS_DEPLOY_PATH,
    )


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
