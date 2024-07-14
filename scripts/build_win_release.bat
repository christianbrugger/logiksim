
@echo off
setlocal

REM ###########################################################

set LS_QT_PATH=C:/Qt/6.7.1/msvc2019_64
set LS_VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community

set LS_BUILD_PROFILE=Release
set LS_BUILD_FOLDER=_build_clang_%LS_BUILD_PROFILE%

REM ###########################################################

set PATH=%LS_QT_PATH%/bin;%PATH%
set CMAKE_PREFIX_PATH=%LS_QT_PATH%/lib/cmake
CALL "%LS_VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" || goto :error


rmdir /Q /S %LS_BUILD_FOLDER%

mkdir %LS_BUILD_FOLDER% || goto :error
cd %LS_BUILD_FOLDER% || goto :error
cmake .. -G Ninja ^
	-DLS_ENABLE_TIME_TRACE=OFF ^
	-DLS_ENABLE_LTO=ON ^
	-DLS_ENABLE_PCH=ON ^
	-DLS_ENABLE_CCACHE=ON ^
    -DCMAKE_C_COMPILER=clang-cl ^
    -DCMAKE_CXX_COMPILER=clang-cl ^
	-DCMAKE_BUILD_TYPE=%LS_BUILD_PROFILE% || goto :error
ninja simulation_srp_gui simulation_srp_test || goto :error

windeployqt6.exe .

call simulation_srp_test.exe || goto :error


REM ###########################################################

goto :EOF
:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%


