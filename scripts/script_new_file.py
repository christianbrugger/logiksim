#!/usr/bin/env python3
"""Create a new *.cpp and *.h file and add it to the cmake project."""

from __future__ import annotations

import argparse
import enum
import textwrap
from pathlib import Path, PurePosixPath, PureWindowsPath

import script_update_source_lists

LS_SRC_ROOT = (Path(__file__).parent.parent / "src").resolve()

LS_NAMESPACE = "logicsim"

LS_HEADER_EXT = "h"
LS_CPP_EXT = "cpp"


class ParameterWrongError(Exception):
    """Raised when a CLI parameter is wrong."""


class TemplateName(enum.Enum):
    """Available templates."""

    GENERIC = enum.auto()
    COMPONENT = enum.auto()


class TemplateType(enum.StrEnum):
    """Available template types."""

    HEADER = LS_HEADER_EXT
    CPP = LS_CPP_EXT


TEMPLATES = {
    TemplateName.GENERIC: {
        TemplateType.HEADER: """
            #ifndef {define}
            #define {define}

            namespace {namespace} {{

            //

            }}

            #endif
        """,
        TemplateType.CPP: """
            #include "{include}"

            namespace {namespace} {{

            //

            }}
        """,
    },
    TemplateName.COMPONENT: {
        TemplateType.HEADER: """
            #ifndef {define}
            #define {define}

            namespace {namespace} {{

            namespace {component} {{

            //

            }}

            }}

            #endif
        """,
        TemplateType.CPP: """
            #include "{include}"

            namespace {namespace} {{

            namespace {component} {{

            //

            }}

            }}
        """,
    },
}


def get_template(name: TemplateName, type_: TemplateType) -> str:
    """Get template with name and type."""
    return textwrap.dedent(TEMPLATES[name][type_]).strip("\n")


def get_component_name(name: PurePosixPath) -> str | None:
    """Get name of component.

    Components are under the ./component/ folder the subfolder defines the name.
    """
    # components need to have at least 3 folders + name
    # example: core/component/layout/wire_store => layout
    min_parts = 4

    if len(name.parts) >= min_parts and name.parts[1] == "component":
        return name.parts[2]

    return None


def create_files(
    name: PurePosixPath,
    header: Path,
    cpp: Path,
    component: str | None,
) -> None:
    """Create cpp and header file at location."""
    replacements = {
        "define": "{}_{}_H".format(LS_NAMESPACE, str(name).replace("/", "_")).upper(),
        "namespace": LS_NAMESPACE.lower(),
        "component": component,
        "include": str(name) + f".{LS_HEADER_EXT}",
    }

    template_name = TemplateName.COMPONENT if component else TemplateName.GENERIC

    for output in (header, cpp):
        template = get_template(template_name, TemplateType(output.suffix.lstrip(".")))
        formatted = template.format(**replacements)

        output.write_text(formatted)
        print("written:", output)


def top_folder(path: PurePosixPath) -> str:
    """Return first folder of a relative path or '' for top level files."""
    # path    ->  'component/layout/wire_store.cpp'
    # parents ->  ['layout', 'component', '.']
    # [-2:]   ->  ['component', '.']
    # [0]     ->  'component'
    result = list(path.parents)[-2:][0].name

    assert str(path).startswith(result)
    return result


def root_folder_names() -> list[str]:
    """Return folder names in the src root."""
    return [folder.name for folder in LS_SRC_ROOT.iterdir() if folder.is_dir()]


def create_file_pair(target_folder: Path, name: PurePosixPath) -> None:
    """Create header and implementation file at the target folder with given name."""
    if name.suffix != "":
        msg = "ERROR: Name cannot have extension"
        raise ParameterWrongError(msg)

    root_folders = root_folder_names()
    if top_folder(name) not in root_folders:
        msg = (
            "ERROR: Given name needs to start with one of the following "
            f"folders: {', '.join(root_folders)}"
        )
        raise ParameterWrongError(msg)

    header = (target_folder / name).with_suffix(f".{LS_HEADER_EXT}")
    cpp = (target_folder / name).with_suffix(f".{LS_CPP_EXT}")
    component = get_component_name(name)

    if header.is_file():
        print("INFO: Header already exists. Aborting")
        return
    if cpp.is_file():
        print("INFO: Cpp file already exists. Aborting")
        return

    create_files(name, header, cpp, component)


def main() -> int:
    """Entry point of this script."""
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    args = parser.parse_args()

    try:
        create_file_pair(LS_SRC_ROOT, PurePosixPath(PureWindowsPath(args.name)))
    except ParameterWrongError as exc:
        print(exc)
        return 1

    script_update_source_lists.main()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
