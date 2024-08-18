# CCache

This tools caches the output of compiler calls and improve build times on clean rebuilds.

## Usage

```bat
ccache -s    # view stats
ccache --show-config

ccache -z    # zero stats
ccache -C    # clear cache
```

## Installation

### Windows

Download the insaller from here:

https://ccache.dev/download.html

Extract files to `C:\Program Files\Ccache`

Add it to System environment variable `PATH`.

### Linux

Simply install it via the package manager.

```bash
sudo apt install ccache
```

## Configuration

### Windows

Use this config at `%LOCALAPPDATA%\ccache\ccache.conf`

```ini
# debug = true
# debug_dir = %LOCALAPPDATA%\ccache\debug

hash_dir = false
sloppiness = pch_defines,time_macros,include_file_mtime,include_file_ctime

# both msvc and clang-cl uses msvc flags
compiler_type = msvc

# otherwise pre-processor is run twice on cache misses doubling incremental build times
# see https://github.com/ccache/ccache/discussions/1420#discussioncomment-8906839
depend_mode = true
```

Verify the config is laoded with

```batch
ccache --show-config
```

### Linux

Use this config at `~/.config/ccache/ccache.conf`

```ini
# debug = true
# debug_dir = ~/.cache/ccache_debug

hash_dir = false
sloppiness = pch_defines,time_macros,include_file_mtime,include_file_ctime

# otherwise pre-processor is run twice on cache misses doubling incremental build times
# see https://github.com/ccache/ccache/discussions/1420#discussioncomment-8906839
depend_mode = true
```

Verify the config is laoded with

```cmd
ccache --show-config
```
