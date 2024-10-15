#!/usr/bin/env python3
"""Checkout all git-submodules of the project.

This is a replacement of the default git checkout process:
    git submodule update --init --recursive --depth 1

The default git checkout takes about 30 minutes locally (doe to download latency).
This script runs in 2 minutes locally by:

    * initializing git modules in parallel.
    * downloading git modules in parallel.
    * only downloading submodules that are used, not all.

Using 'git submodule update ...' in Github Actions (4-cores):
    ubuntu-24.04      5:17 min
    windows-2022      7:10 min

Using 'checkout.py' in Github Actions (4-cores):
    ubuntu-24.04      0:15 min   (21.1x speedup)
    windows-2022      1:05 min   ( 6.6x speedup)

Using 'checkout.py' with caching in Github Actions (4-cores):
    ubuntu-24.04      0:04 min   (79x speedup)
    windows-2022      0:14 min   (31x speedup)
"""

from __future__ import annotations

import argparse
import asyncio
import re
import shlex
import subprocess
import typing
from dataclasses import dataclass
from pathlib import Path

if typing.TYPE_CHECKING:
    from collections.abc import Iterable


MODULES = [
    "external/asmjit",
    "external/benchmark",
    "external/blend2d",
    "external/boost",
    "external/expected",
    "external/fmt",
    "external/gcem",
    "external/glaze",
    "external/googletest",
    "external/GSL",
    "external/harfbuzz",
    "external/my_folly",
    "external/range-v3",
    "external/svg2b2d",
    "external/unordered_dense",
    "external/whereami",
    "external/zlib",
]

MODULE_LLVM = "external/llvm-project"

SUB_MODULES = {
    "external/boost": [
        # only used for configure and private libs
        "tools/cmake",
        "libs/align",
        # taken from 'compile_commands.json' produced by cmake
        # (requires that all packages are already checked out)
        "libs/algorithm",
        "libs/any",
        "libs/array",
        "libs/assert",
        "libs/atomic",
        "libs/bind",
        "libs/chrono",
        "libs/concept_check",
        "libs/config",
        "libs/container",
        "libs/container_hash",
        "libs/conversion",
        "libs/core",
        "libs/date_time",
        "libs/describe",
        "libs/detail",
        "libs/dynamic_bitset",
        "libs/endian",
        "libs/exception",
        "libs/filesystem",
        "libs/function",
        "libs/function_types",
        "libs/functional",
        "libs/fusion",
        "libs/geometry",
        "libs/integer",
        "libs/intrusive",
        "libs/io",
        "libs/iostreams",
        "libs/iterator",
        "libs/lexical_cast",
        "libs/logic",
        "libs/math",
        "libs/move",
        "libs/mp11",
        "libs/mpl",
        "libs/multiprecision",
        "libs/numeric/conversion",
        "libs/optional",
        "libs/phoenix",
        "libs/polygon",
        "libs/pool",
        "libs/predef",
        "libs/preprocessor",
        "libs/proto",
        "libs/qvm",
        "libs/random",
        "libs/range",
        "libs/ratio",
        "libs/rational",
        "libs/regex",
        "libs/safe_numerics",
        "libs/scope",
        "libs/serialization",
        "libs/smart_ptr",
        "libs/spirit",
        "libs/static_assert",
        "libs/system",
        "libs/thread",
        "libs/throw_exception",
        "libs/tokenizer",
        "libs/tuple",
        "libs/type_index",
        "libs/type_traits",
        "libs/typeof",
        "libs/unordered",
        "libs/utility",
        "libs/variant",
        "libs/variant2",
        "libs/winapi",
    ],
}


GIT_ROOT_FOLDER = Path(__file__).parent.parent.resolve()
CALL_SEMA = asyncio.Semaphore(8)


@dataclass(frozen=True)
class CallResult:
    """Store result of subprocess execution."""

    stdout: str
    stderr: str
    returncode: int


async def call_once(
    cmd: str,
    *,
    do_print: bool = False,
    checked: bool = True,
) -> CallResult:
    """Call the subprocess once and store the result."""
    async with CALL_SEMA:
        process = await asyncio.create_subprocess_exec(
            *shlex.split(cmd),
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            cwd=GIT_ROOT_FOLDER,
        )
        stdout_raw, stderr_raw = await process.communicate()

    stdout = stdout_raw.decode("utf-8")
    stderr = stderr_raw.decode("utf-8")
    assert process.returncode is not None

    if do_print:
        print(f"CALL ({process.returncode}): {cmd}")
    if do_print and len(stdout) > 0:
        print("stdout:")
        print(stdout)
    if do_print and len(stderr) > 0:
        print("stderr:")
        print(stderr)

    if checked and process.returncode != 0:
        raise subprocess.CalledProcessError(
            returncode=process.returncode,
            cmd=cmd,
            output=stdout,
            stderr=stderr,
        )

    return CallResult(stdout=stdout, stderr=stderr, returncode=process.returncode)


STANDARD_RETRY_TAGS = (
    re.compile(r"could not lock config file '[^:]+': File exists"),
    re.compile(r"unable to access '[^.]+': Permission denied"),
    re.compile(r"could not lock config file"),
    re.compile(r"unknown error occurred while reading the configuration files"),
)


def matches_retry_tag(stderr: str, retry_tags: Iterable[re.Pattern[str]]) -> bool:
    """Check if stderr contains a retry_tag."""
    return any(tag.search(stderr) for tag in retry_tags)


@dataclass(frozen=True)
class RetryConfig:
    """Configure the retry logic."""

    retry_tags: Iterable[re.Pattern[str]] = STANDARD_RETRY_TAGS
    retry_count: int = 5
    retry_delay_seconds: float = 0.1


DEFAULT_RETRY_CONFIG = RetryConfig()


async def call(
    cmd: str,
    *,
    do_print: bool = False,
    checked: bool = True,
    config: RetryConfig = DEFAULT_RETRY_CONFIG,
) -> CallResult:
    """Call the given command with retry functionality."""
    call_count = 0

    while True:
        result = await call_once(cmd, do_print=do_print, checked=False)
        call_count += 1

        if call_count <= config.retry_count + 1 and matches_retry_tag(
            result.stderr,
            config.retry_tags,
        ):
            if do_print:
                print("WAIT", cmd)
            await asyncio.sleep(config.retry_delay_seconds)
            continue

        if result.returncode != 0 and checked:
            raise subprocess.CalledProcessError(
                returncode=result.returncode,
                cmd=cmd,
                output=result.stdout,
                stderr=result.stderr,
            )

        return result


def to_context(repo_path: Path | str | None) -> str:
    """Convert path to context git argument."""
    return f"-C {repo_path}" if repo_path is not None else ""


@dataclass
class Submodule:
    """Git submodule info."""

    name: str
    is_initialized: bool


def match_submodule_output_line(line: str) -> None | Submodule:
    """Parse the submodule from single output line.

    Input:
        +fda4e9a67ab375e24682b8babb14fd7d5296927d boost (boost-1.71.0)
    Output:
        Submodule(name="boost")
    """
    regex = re.compile(r"^([^ ]+) ([^ ]+)( ([^ ]+))?$")

    match = re.match(regex, line.strip())

    if match is None:
        return None

    return Submodule(name=match.group(2), is_initialized=match.group(4) is not None)


def match_submodule_output(stdout: str) -> list[Submodule]:
    """Parse submodule from git output."""
    return [
        submodule
        for line in stdout.split("\n")
        if (submodule := match_submodule_output_line(line)) is not None
    ]


async def get_submodules_status(parent_repo: Path | None = None) -> list[Submodule]:
    """Query status of all submodule for the repo path."""
    cmd = f"git {to_context(parent_repo)} submodule"

    result = await call(cmd)
    return match_submodule_output(result.stdout)


def parse_config_as_dict(stdout: str, key: str) -> dict[str, str]:
    """Parse available config output to dictionary.

    Input: (key = active)
        submodule.external/svg2b2d.active true
    Output:
        {"external/svg2b2d": "true"}
    """
    regex = re.compile(rf"^submodule\.([^.]+)\.{key} (.+)$")

    return {
        match.group(1): match.group(2).strip()
        for line in stdout.split("\n")
        if (match := re.match(regex, line.strip())) is not None
    }


async def get_available_submodules(repo_path: str | None = None) -> dict[str, str]:
    """Query the available submodules from git config."""
    prefix = repo_path + "/" if repo_path is not None else ""
    cmd = f"git config -f {prefix}.gitmodules --get-regexp path"
    result = await call(cmd)

    return parse_config_as_dict(result.stdout, "path")


async def get_initialized_submodules(repo_path: str | None = None) -> list[str]:
    """Get the initialized submodules at the repo path."""
    cmd = f"git {to_context(repo_path)} config --local --get-regexp (active|url)"
    result = await call(cmd)

    params_active = parse_config_as_dict(result.stdout, "active")
    params_url = parse_config_as_dict(result.stdout, "url")

    # see https://git-scm.com/docs/gitsubmodules#_active_submodules
    return [
        module
        for module in sorted(set(params_active).union(params_url))
        if params_active.get(module, "false").lower() == "true"
        or (module not in params_active and len(params_url.get(module, "")) > 0)
    ]


async def init_submodule(
    submodule_name: str,
    parent_repo: str | Path | None = None,
) -> None:
    """Initialize the submodule with given name and parent repo path."""
    cmd = f'git {to_context(parent_repo)} submodule init "{submodule_name}"'
    await call(cmd)
    print(f"initalized module '{submodule_name}'{repo_path_to_msg(parent_repo)}")


async def deinit_submodule(
    submodule_name: str,
    parent_repo: str | Path | None = None,
) -> None:
    """Uninitialize the submodule with given name and parent repo path."""
    cmd = f'git {to_context(parent_repo)} submodule deinit -f "{submodule_name}"'
    await call(cmd)
    print(f"de-initalized module '{submodule_name}'{repo_path_to_msg(parent_repo)}")


@dataclass(frozen=True)
class InitChangeset:
    """The required changes to all the git submodules."""

    expected_paths: list[str]  # path
    available_paths: dict[str, str]  # name: path
    initialized_paths: dict[str, str]  # name: path
    to_init: set
    to_deinit: set


async def get_initialize_changeset(
    expected_paths: list[str],
    parent_repo: str | None = None,
    *,
    do_print: bool = False,
) -> InitChangeset:
    """Query and calculate the status and necessary changes to all git submodules."""
    available_paths: dict[str, str] = await get_available_submodules(parent_repo)
    initialized_names: list[str] = await get_initialized_submodules(parent_repo)
    initialized_paths = {
        name: available_paths[name]
        for name in initialized_names
        if name in available_paths
    }

    to_init = (
        set(expected_paths)
        .intersection(available_paths.values())
        .difference(initialized_paths.values())
    )
    to_deinit = (
        set(initialized_paths.values())
        .intersection(available_paths.values())
        .difference(expected_paths)
    )

    if do_print:
        print("Available =", available_paths)
        print("Initialized =", initialized_names)
        print("To initialize =", to_init)
        print("To de-initialize =", to_deinit)

    return InitChangeset(
        expected_paths,
        available_paths,
        initialized_paths,
        to_init,
        to_deinit,
    )


def parent_repo_to_msg(parent_repo: str | None) -> str:
    """Convert parent repo to human readable string."""
    if parent_repo is None:
        return "modules"
    return f"submodules of '{parent_repo}'"


def repo_path_to_msg(parent_repo: str | None) -> str:
    """Convert repo path to human readable string."""
    if parent_repo is None:
        return ""
    return f" of '{parent_repo}'"


async def initialize_modules(
    expected_submodules: list[str],
    parent_repo: str | None = None,
) -> list[str]:
    """Initialize and uninitialize submodules so it matches the expected set."""
    changes = await get_initialize_changeset(expected_submodules, parent_repo)

    if len(changes.to_init) + len(changes.to_deinit) == 0:
        print(f"All {parent_repo_to_msg(parent_repo)} initalized.")
        return sorted(changes.initialized_paths.values())

    async with asyncio.TaskGroup() as tg:
        for path in changes.to_init:
            tg.create_task(init_submodule(path, parent_repo))
        for path in changes.to_deinit:
            tg.create_task(deinit_submodule(path, parent_repo))

    # check status now
    result = await get_initialize_changeset(expected_submodules, parent_repo)
    if len(result.to_init) + len(result.to_deinit) > 0:
        msg = "Init / Deinit failed. Logic error or git version not compatible."
        raise RuntimeError(msg)

    return sorted(result.initialized_paths.values())


async def update_module(module_path: str, repo_path: str | None = None) -> None:
    """Update a single module to the specified version in the config."""
    cmd = f"git {to_context(repo_path)} submodule update --depth 1 {module_path}"
    result = await call(cmd)
    if len(result.stdout) > 0:
        print(f"Updated '{module_path}'{repo_path_to_msg(repo_path)}")


async def update_modules(module_paths: list[str], repo_path: str | None = None) -> None:
    """Update given modules to the specified version in the config."""
    async with asyncio.TaskGroup() as tg:
        for module in module_paths:
            tg.create_task(update_module(module, repo_path))


async def initialize_and_update_modules(
    module_paths: list[str],
    repo_path: str | None = None,
) -> None:
    """Initialize, deinitialize and update the given submodules of the parent repo."""
    available_paths = await initialize_modules(module_paths, repo_path)
    await update_modules(available_paths, repo_path)


async def main_async(modules: list[str], sub_modules: dict[str, list[str]]) -> None:
    """Initialize, deinitialize and update all submodules recursively."""
    # modules
    await initialize_and_update_modules(modules)

    # sub-modules
    async with asyncio.TaskGroup() as tg:
        for module in sub_modules:
            if module in modules:
                tg.create_task(initialize_and_update_modules(sub_modules[module], module))


def main() -> int:
    """Process command line parameters and sync all submodules process."""
    parser = argparse.ArgumentParser(
        description="Check out and sync all git submodules of this project.",
    )
    parser.add_argument(
        "--libc++",
        action="store_true",
        help="Checkout the Clang c++ standard library. "
        "This is only required when building with thread or memory santiziers."
        "Disable by default.",
    )
    args = parser.parse_args()

    modules = MODULES
    sub_modules = SUB_MODULES
    if getattr(args, "libc++"):
        modules.append(MODULE_LLVM)

    return_code = 0
    try:
        asyncio.run(main_async(modules, sub_modules))
    except* subprocess.CalledProcessError as group:
        for exc in group.exceptions:
            print("ERROR", exc)
        # TODO: explain why no return 1
        return_code = 1
    return return_code


if __name__ == "__main__":
    raise SystemExit(main())
