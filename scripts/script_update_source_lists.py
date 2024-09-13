#!/usr/bin/env python3
"""Update the 'source_list.cmake' files with actual source."""

from __future__ import annotations

import itertools
import re
from pathlib import Path, PurePosixPath

LS_SRC_ROOT = (Path(__file__).parent.parent / "src").resolve()
LS_SOURCE_EXTENSION = "cpp"
LS_CMAKE_SOURCE_FILENAME = "source_list.cmake"


def to_dir_group(path: PurePosixPath) -> tuple[list[str], str]:
    """Sort directories before files when sorting with this key function."""
    # adding chr(255) puts files at the end of directories
    return [str(comp) for comp in path.parents][::-1] + [chr(255)], path.name


def get_source_paths(folder: Path) -> list[PurePosixPath]:
    """Return sorted list of sources in the folder."""
    sources = [
        PurePosixPath(file.relative_to(folder))
        for file in folder.rglob(f"*.{LS_SOURCE_EXTENSION}")
    ]

    return sorted(sources, key=to_dir_group)


def top_folder(path: PurePosixPath) -> str:
    """Return first folder of a relative path or '' for top level files."""
    result = list(path.parents)[-2:][0].name

    assert str(path).startswith(result)
    return result


def format_variable(
    name: str,
    sources: list[PurePosixPath],
    indent: str = " " * 4,
) -> str:
    """Create cmake variable from list of sources."""
    lines = [f"set({name}"]

    for previous, source in itertools.pairwise([None, *sources]):
        assert source is not None

        # group by top folder with new lines
        if previous is not None and top_folder(previous) != top_folder(source):
            lines.append("")

        lines.append(f"{indent}{source}")

    lines.append(")")
    return "\n".join(lines)


def get_first_variable_name(content: str) -> str:
    """Return first cmake variable in the content."""
    return re.search(r"set\(([\w_]+)\n", content).group(1)


def update_source_list(folder: Path) -> None:
    """Update the source list in the folder if it exists."""
    output = folder / LS_CMAKE_SOURCE_FILENAME

    if not output.is_file():
        return

    content = output.read_text()

    sources = get_source_paths(folder)
    variable_name = get_first_variable_name(content)
    variable = format_variable(variable_name, sources)

    new_content = re.sub(rf"set\({variable_name}[\s\w/_.]+\)", variable, content)
    output.write_text(new_content)

    # print(f"updated: {output.relative_to(LS_SRC_ROOT)}")


def main() -> None:
    """Entry point of this script."""
    folders = list(filter(Path.is_dir, LS_SRC_ROOT.iterdir()))

    for folder in folders:
        update_source_list(folder)


if __name__ == "__main__":
    raise SystemExit(main())
