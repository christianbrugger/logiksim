#!/usr/bin/env python3
""""
Checkout all git-submodules fast and in parallel.
"""
from __future__ import annotations

import asyncio
from dataclasses import dataclass
from pathlib import Path
import re
import shlex
import subprocess


MODULES = [
    "external/abseil-cpp",
    "external/asmjit",
    "external/benchmark",
    "external/blend2d",
    "external/boost",
    "external/cppcodec",
    "external/expected",
    "external/fmt",
    "external/gcem",
    "external/glaze",
    "external/googletest",
    "external/GSL",
    "external/harfbuzz",
    # "external/llvm-project",
    "external/my_folly",
    "external/range-v3",
    "external/svg2b2d",
    "external/unordered_dense",
    "external/whereami",
    "external/zlib",
]

SUB_MODULES = {
    "external/boost": [
        # only used for configure and private libs
        "tools/cmake",
        "libs/align",
        # taken from 'compile_commands.json' produced by cmake
        # this however requires that all required packages are already checked out
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
    ]
}


GIT_ROOT_FOLDER = Path(__file__).parent.parent.resolve()
CALL_SEMA = asyncio.Semaphore(8)


@dataclass(frozen=True)
class CallResult:
    stdout: str
    stderr: str
    returncode: int


async def call_once(cmd: str, *, do_print: bool = True,
                    checked: bool = True) -> CallResult:
    async with CALL_SEMA:
        process = await asyncio.create_subprocess_exec(
            *shlex.split(cmd),
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            cwd=GIT_ROOT_FOLDER,
        )
        stdout_raw, stderr_raw = await process.communicate()

    stdout = stdout_raw.decode('utf-8')
    stderr = stderr_raw.decode('utf-8')

    if do_print:
        print(f"CALL ({process.returncode}): {cmd}")
    if do_print and len(stdout) > 0:
        print(f"stdout:")
        print(stdout)
    if do_print and len(stderr) > 0:
        print(f"stderr:")
        print(stderr)

    if checked and process.returncode != 0:
        raise subprocess.CalledProcessError(
            returncode=process.returncode, cmd=cmd, output=stdout, stderr=stderr)

    return CallResult(stdout=stdout, stderr=stderr, returncode=process.returncode)


STANDARD_RETRY_TAGS = (
    re.compile(r"could not lock config file '[^:]+': File exists"),
    re.compile(r"unable to access '[^.]+': Permission denied"),
    re.compile(r"could not lock config file"),
    re.compile(r"unknown error occurred while reading the configuration files"),
)


def matches_retry_tag(stderr, retry_tags: tuple[re.Pattern[str]]) -> bool:
    return any(tag.search(stderr) for tag in retry_tags)


async def call(cmd: str, *, do_print: bool = True, checked: bool = True,
               retry_tags: tuple[re.Pattern[str]] = STANDARD_RETRY_TAGS,
               retry_count: int = 5,
               retry_delay_seconds: float = 0.1) -> CallResult:
    call_count = 0

    while True:
        result = await call_once(cmd, do_print=do_print, checked=False)
        call_count += 1

        if call_count <= retry_count + 1 and matches_retry_tag(result.stderr, retry_tags):
            print("WAIT", cmd)
            await asyncio.sleep(retry_delay_seconds)
            continue

        if result.returncode != 0 and checked:
            raise subprocess.CalledProcessError(
                returncode=result.returncode, cmd=cmd,
                output=result.stdout, stderr=result.stderr)

        return result


def to_context(repo_path: str | None) -> str:
    return f"-C {repo_path}" if repo_path is not None else ""


@dataclass
class Submodule:
    name: str
    is_initialized: True


def match_submodule_output_line(line: str) -> None | Submodule:
    """
    Input:
        +fda4e9a67ab375e24682b8babb14fd7d5296927d boost (boost-1.71.0)
    Output:
        Submodule(name="boost")
    """
    regex = re.compile(r'^([^ ]+) ([^ ]+)( ([^ ]+))?$')

    match = re.match(regex, line.strip())

    if match is None:
        return None

    return Submodule(name=match.group(2),
                     is_initialized=match.group(4) is not None)


def match_submodule_output(stdout: str) -> list[Submodule]:
    return [
        submodule
        for line in stdout.split('\n')
        if (submodule := match_submodule_output_line(line)) is not None
    ]


async def get_submodules_status(parent_repo: Path | None = None) -> list[Submodule]:
    cmd = f'git {to_context(parent_repo)} submodule'

    result = await call(cmd)
    return match_submodule_output(result.stdout)


def parse_config_as_dict(stdout: str, key: str) -> dict[str, str]:
    """
    Input: (key = active)
        submodule.external/svg2b2d.active true
    Output:
        {"external/svg2b2d": "true"}
    """
    regex = re.compile(rf'^submodule\.([^.]+)\.{key} (.+)$')

    return {
        match.group(1): match.group(2).strip()
        for line in stdout.split('\n')
        if (match := re.match(regex, line.strip())) is not None
    }


async def get_available_submodules(repo_path: str | None = None) -> dict[str, str]:
    prefix = repo_path + '/' if repo_path is not None else ''
    cmd = f'git config -f {prefix}.gitmodules --get-regexp path'
    result = await call(cmd, do_print=False)

    # {name: path}
    return parse_config_as_dict(result.stdout, 'path')


async def get_initialized_submodules(repo_path: str | None = None) -> list[str]:
    cmd = f'git {to_context(repo_path)} config --local --get-regexp (active|url)'
    result = await call(cmd, do_print=False)

    params_active = parse_config_as_dict(result.stdout, 'active')
    params_url = parse_config_as_dict(result.stdout, 'url')

    # see https://git-scm.com/docs/gitsubmodules#_active_submodules
    return [
        module
        for module in sorted(set(params_active).union(params_url))
        if params_active.get(module, "false").lower() == 'true' or
        (module not in params_active and len(params_url.get(module, '')) > 0)
    ]


async def init_submodule(submodule_name: str,
                         parent_repo: Path | None = None) -> None:
    cmd = f'git {to_context(parent_repo)} submodule init "{submodule_name}"'
    await call(cmd)


async def deinit_submodule(submodule_name: str,
                           parent_repo: Path | None = None) -> None:
    cmd = f'git {to_context(parent_repo)} submodule deinit -f "{submodule_name}"'
    await call(cmd)


@dataclass(frozen=True)
class InitChangeset:
    available_paths: dict[str, str]  # name: path
    initialized_paths: dict[str, str]  # name: path
    to_init: set
    to_deinit: set


async def get_initialize_changeset(expected_paths: list[str],
                                   parent_repo: str | None = None,
                                   do_print: bool = False) -> InitChangeset:
    available_paths: dict = await get_available_submodules(parent_repo)
    initialized_names: list = await get_initialized_submodules(parent_repo)
    initialized_paths = {
        name: available_paths[name]
        for name in initialized_names
        if name in available_paths
    }

    to_init = set(expected_paths) \
        .intersection(available_paths.values()) \
        .difference(initialized_paths.values())
    to_deinit = set(initialized_paths.values()) \
        .intersection(available_paths) \
        .difference(expected_paths)

    if do_print:
        print("Available =", available_paths)
        print("Initialized =", initialized_names)
        print("To initialize =", to_init)
        print("To de-initialize =", to_deinit)

    return InitChangeset(
        available_paths, initialized_paths, to_init, to_deinit)


async def initialize_modules(expected_submodules: list[str],
                             parent_repo: str | None = None) -> list[str]:
    changes = await get_initialize_changeset(
        expected_submodules, parent_repo, do_print=True)

    if len(changes.to_init) + len(changes.to_deinit) == 0:
        return sorted(changes.initialized_paths.values())

    async with asyncio.TaskGroup() as tg:
        for path in changes.to_init:
            tg.create_task(init_submodule(path, parent_repo))
        for path in changes.to_deinit:
            tg.create_task(deinit_submodule(path, parent_repo))

    # check status now
    result = await get_initialize_changeset(expected_submodules, parent_repo)
    if len(result.to_init) + len(result.to_deinit) > 0:
        raise Exception("Init / Deinit failed")

    return sorted(result.initialized_paths.values())


async def update_modules(module_paths: list[str],
                         repo_path: str | None = None) -> None:
    async with asyncio.TaskGroup() as tg:
        for module in module_paths:
            cmd = f"git {to_context(repo_path)} submodule update --depth 1 {module}"
            tg.create_task(call(cmd))


async def initialize_and_update_modules(module_paths: list[str],
                                        repo_path: str | None = None) -> None:
    available_paths = await initialize_modules(module_paths, repo_path)
    await update_modules(available_paths, repo_path)


async def main_async():
    # modules
    await initialize_and_update_modules(MODULES)

    # sub-modules
    async with asyncio.TaskGroup() as tg:
        for module in SUB_MODULES:
            if module in MODULES:
                tg.create_task(initialize_and_update_modules(SUB_MODULES[module], module))


def main() -> int:
    returncode = 0

    try:
        asyncio.run(main_async())
    except ExceptionGroup as group:
        for exc in group.exceptions:
            if isinstance(exc, subprocess.CalledProcessError):
                print("ERROR", exc)
                returncode = 1
            else:
                raise
    except subprocess.CalledProcessError as exc:
        print("ERROR", exc)
        returncode = 1

    return returncode


if __name__ == "__main__":
    raise SystemExit(main())
