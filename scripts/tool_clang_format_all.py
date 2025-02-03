#!/usr/bin/env python3

import asyncio
import os
from pathlib import Path


CLANG_FORMAT = "clang-format-18"
SOURCE_FOLDERS = ("src", "test")
EXTENSIONS = ("*.h", "*.cpp")
IGNORE = [
    "src/main_winui3/Generated Files",
    "src/main_winui3/packages",
]


class SharedState:
    def __init__(self, root: Path, total_count: int):
        self.root = root
        self.total_count: int = total_count

        self.semaphore = asyncio.Semaphore(os.cpu_count() * 4)  # mostly I/O bound
        self.processed = 0


async def format_file(state: SharedState, filename: Path):
    async with state.semaphore:
        process = await asyncio.create_subprocess_exec(CLANG_FORMAT, "-i", str(filename))
        await process.wait()

        state.processed += 1
        print(
            f"[{state.processed}/{state.total_count}] {filename.relative_to(state.root)}"
        )

        if process.returncode != 0:
            raise Exception("Process excited with non-zero returncode.")


async def process_all_files(root: Path, source_files: list[Path]) -> None:
    state = SharedState(root, len(source_files))

    async with asyncio.TaskGroup() as tg:
        for filename in source_files:
            tg.create_task(format_file(state, filename))


def is_ignored(project_root: Path, filepath: Path) -> bool:
    return any(filepath.is_relative_to(project_root / ignore) for ignore in IGNORE)


def get_source_files(project_root: Path) -> list[Path]:
    return sorted(
        [
            filepath
            for subfolder in SOURCE_FOLDERS
            for extension in EXTENSIONS
            for filepath in (project_root / subfolder).rglob(extension)
            if not is_ignored(project_root, filepath)
        ]
    )


def main():
    project_root = Path(__file__).parent.parent
    files = get_source_files(project_root)

    asyncio.run(process_all_files(project_root, files))


if __name__ == "__main__":
    raise SystemExit(main())
