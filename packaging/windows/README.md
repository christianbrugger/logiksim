# Windows Installer



## Create new Installer

#### 1. Update `inno-setup.iss` version

Update the version to the desired LogikSim version.

#### 2. Run `bundle_installer.py`

Open a `Developer Powershell for VS 2022`. Make sure all the dependencies are available on the path below.

Then run the following in order:

```powershell
python3 script_create_installer.py clean

python3 script_create_installer.py all
```

The final output is under `temp`. For example:

```
LogikSim_2.2.0_win_x64.exe
LogikSim_2.2.0_win_x64_installer.zip
LogikSim_2.2.0_win_x64_portable.zip
```



##### Debugging

For debugging individual steps can be executed:

```
python3 script_create_installer.py configure
python3 script_create_installer.py build
python3 script_create_installer.py deploy
python3 script_create_installer.py package
```







## Dependencies



### Build Dependencies

All build dependencies need to be available from the PATH.

* CMake
* Ninja
* Visual Studio 2022
* Clang
* Qt binary folder



### Inno Setup

Main tool to create the installer.

https://jrsoftware.org/isinfo.php



`iscc.exe` needs to be available in the `PATH`. Usually:

```
C:\Program Files (x86)\Inno Setup 6
```



*tested with 6.3.3*



### Resource Hacker

Add icon to executable.

https://www.angusj.com/resourcehacker/



`ResourceHacker.exe` needs to be available in the `PATH`. Usually:

```
C:\Program Files (x86)\Resource Hacker
```



*tested with 5.2.7*



### 7zip

To zip the installers.

https://www.7-zip.org/



`7z.exe` needs to be available in the `PATH`. Usually:

```
C:\Program Files\7-Zip
```



*tested with 24.08*



### Python 3.12+

Required to run the bundling script.

https://www.python.org/downloads/

*tested with 3.12.4*
