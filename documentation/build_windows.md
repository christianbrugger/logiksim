# Build Instructions for Windows



## Build Dependencies

External tools needed for building, not included in this repository.



### Visual Studio Community 2022

Download the Installer from here:

 https://visualstudio.microsoft.com/vs/community/

Make sure to install the following components:

* **MSVC v134** - VS 2022 C++ x64/x86 build tools (Latest)
* **C++ CMake** tools for Windows
* **C++ Clang** tools for Windows (17.0.3 - x64/x86)
* **Windows 11 SDK** (10.0.26100.0)

![image-20240616105203692](.images/image-20240616105203692.png)

*tested with Visual Studio Community 17.10.2*

### Python 3.11+

Download the Installer from here:

https://www.python.org/downloads/

Make sure `python` is available in the PATH.

*tested with Python 3.12.4*

### Git

Download the Installer from here:

https://git-scm.com/download/win/

Make sure `git` is avaiable in the PATH.

*tested with 2.45.2*

### Qt 6.7+

Qt is not build from source and is best installed with the Qt Installer:

https://www.qt.io/download-qt-installer-oss

The only required package for building is:

* MSVC 2019 64-bit

Recommended for debugging:

* Sources
* Qt Debug Information Files

Anything else can be disabled

![image-20240616113641673](.images/image-20240616113641673.png)

*tested with Qt 6.7.1*



## External Libraries

External libraries are part of the git repository as submodules.

Check out the required submodules by running:

```cmd
python3 external/checkout.py
```

The script initializes and updates all required submodules for the platform to the SHA specified by the current commit.

As we have almost 100 submodules the script parallizes the git operations and should finish within 1-2 minutes depending on the download speed.

Re-run the script in case a different tag / branch / commit is checked out.



## Build Scripts

If you installed MSVC and Qt in the standard locations and want to make a release build you can use the script

 `scripts/build_win_release.bat`

The result will be in the `logiksim/_build_win_release` folders.



## Build Steps

Alternatively for custom builds you can follow the build steps here.

#### Environment Variables

Add Qt to `PATH` and `CMAKE_PREFIX_PATH`. Replace the version as needed, e.g `6.7.1`:

```cmd
set PATH=C:/Qt/6.x.x/msvc2019_64/bin;%PATH%
set CMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2019_64/lib/cmake
```

Activate the MSVC Compiler. This will make `cmake`, `ninja`, `clang-cl` avaiable:

```cmd
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### Prepare Build Folder

Create an emtpy build folder inside the logiksim git repostiroy root:

```cmd
mkdir build
cd build
```

#### Configure

First configure with cmake.

For **debug** builds use:

```cmd
cmake .. -G Ninja ^
	-DCMAKE_BUILD_TYPE=Debug ^
	-DLS_ENABLE_CCACHE=OFF ^
	-DLS_ENABLE_PCH=ON ^
	-DLS_ENABLE_LTO=OFF ^
    -DCMAKE_C_COMPILER=clang-cl ^
    -DCMAKE_CXX_COMPILER=clang-cl
```

For **release** builds use:

```cmd
cmake .. -G Ninja ^
	-DCMAKE_BUILD_TYPE=Release ^
	-DLS_ENABLE_CCACHE=OFF ^
	-DLS_ENABLE_PCH=ON ^
	-DLS_ENABLE_LTO=ON ^
    -DCMAKE_C_COMPILER=clang-cl ^
    -DCMAKE_CXX_COMPILER=clang-cl
```

Configuration should take 30 - 60 seconds.

#### Build

Run the build

```cmd
ninja
```

This will build all dependencies from source (except Qt) and then build LogikSim.

A complete build from scratch should take about 3-5 minutes depending on the machine.

#### Execution

This should build all the binaries you can run the GUI with:

```cmd
simulation_srp_gui
```

Run the tests with

```
simulation_srp_test
```

Run the benchmark with

```
simulation_srp_benchmark
```

Run the non-gui experimental main (used for testing out new features directly)

```
simulation_srp_main
```



