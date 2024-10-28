"""Build LogikSim and create Windows installer."""

from __future__ import annotations

import argparse
import contextlib
import os
import re
import shutil
import subprocess
from pathlib import Path

#
# FOLDERS
#

LS_ROOT = Path(__file__).parent.parent.parent.resolve()
LS_SCRIPT_DIR = Path(__file__).parent.resolve()
LS_TEMP_PATH = LS_SCRIPT_DIR / "temp"

LS_BUILD_PATH = LS_TEMP_PATH / "build"
LS_DEPLOY_PATH = LS_TEMP_PATH / "deploy"

#
# CONFIGURATION
#

LS_CMAKE_CONFIG = "win-clang-release-package"
LS_GUI_BINARY_SRC = "ls_gui.exe"
LS_GUI_BINARY_DST = "logiksim.exe"
LS_TEST_BINARIES = ["ls_test_core.exe", "ls_test_gui.exe"]
LS_RESOURCE_DIRS = ["resources/fonts", "resources/icons"]

# folder under $env:VCToolsRedistDir
LS_REDIST_FOLDER = "x64/Microsoft.VC143.CRT"

LS_ICON_SRC = LS_ROOT / "resources" / "icons" / "own" / "app_icon_256.ico"
LS_ICON_DST = LS_DEPLOY_PATH / "logiksim.ico"

LS_INNO_SETUP = "inno_setup.iss"


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

    #
    # application files
    shutil.copy(LS_BUILD_PATH / LS_GUI_BINARY_SRC, LS_DEPLOY_PATH / LS_GUI_BINARY_DST)
    for resource_dir in LS_RESOURCE_DIRS:
        shutil.copytree(LS_BUILD_PATH / resource_dir, LS_DEPLOY_PATH / resource_dir)

    #
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

    #
    # vc redist
    redist_base = os.getenv("VCToolsRedistDir")
    assert redist_base is not None

    redist_dir = (Path(redist_base) / LS_REDIST_FOLDER).resolve()
    folder_only_contains_dll(redist_dir)

    for item in redist_dir.glob("*.dll"):
        shutil.copy(item, LS_DEPLOY_PATH / item.name)

    #
    # app icon
    shutil.copy(LS_ICON_SRC, LS_ICON_DST)
    subprocess.run(
        [
            "ResourceHacker",
            "-open",
            LS_GUI_BINARY_DST,
            "-save",
            LS_GUI_BINARY_DST,
            "-action",
            "add",
            "-res",
            LS_ICON_DST.name,
            "-mask",
            "ICONGROUP,MAINICON,",
        ],
        check=True,
        cwd=LS_DEPLOY_PATH,
    )


def action_package() -> None:
    """Package binary and ressources into windows installer with Inno Setup."""
    subprocess.run(
        [
            "iscc",
            LS_INNO_SETUP,
        ],
        check=True,
        cwd=LS_SCRIPT_DIR,
    )

    # define zip names
    output_names = list(LS_TEMP_PATH.glob("LogikSim_*_win_x64.exe"))
    assert len(output_names) == 1
    installer_exe = output_names[0]
    version = re.match(r"^LogikSim_([0-9.]+)_win_x64.exe", installer_exe.name).group(1)

    installer_zip = LS_TEMP_PATH / f"LogikSim_{version}_win_x64_installer.zip"
    portable_zip = LS_TEMP_PATH / f"LogikSim_{version}_win_x64_portable.zip"
    portable_folder = LS_TEMP_PATH / f"LogikSim_{version}"

    # zip installer
    subprocess.run(
        [
            "7z",
            "a",
            "-tzip",
            "-mm=Deflate",
            "-mx=9",
            installer_zip,
            installer_exe,
        ],
        check=True,
        cwd=LS_TEMP_PATH,
    )

    # zip portable
    with contextlib.suppress(FileNotFoundError):
        shutil.rmtree(portable_folder)
    shutil.copytree(LS_DEPLOY_PATH, portable_folder)
    subprocess.run(
        [
            "7z",
            "a",
            "-tzip",
            "-mm=Deflate",
            "-mx=9",
            portable_zip,
            portable_folder,
        ],
        check=True,
        cwd=LS_TEMP_PATH,
    )


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "action",
        choices=["clean", "all", "configure", "build", "deploy", "package"],
        help="Action to perform. Need to be run in order.",
    )
    args = parser.parse_args()

    if args.action in ("clean",):
        action_clean()
    if args.action in ("configure", "all"):
        action_configure()
    if args.action in ("build", "all"):
        action_build()
    if args.action in ("deploy", "all"):
        action_deploy()
    if args.action in ("package", "all"):
        action_package()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
