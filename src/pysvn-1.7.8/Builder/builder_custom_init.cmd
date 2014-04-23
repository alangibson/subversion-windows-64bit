@echo off
rem figure out where we are
for %%I in ( %0\..\.. ) do set WORKDIR=%%~fI

set PY_MAJ=2
if not "%1" == "" set PY_MAJ=%1
set PY_MIN=7
if not "%2" == "" set PY_MIN=%2
if not "%3" == "" set BUILD_TYPE=%3
if not "%4" == "" set SVN_VER_MAJ_MIN=%4


if "%SVN_VER_MAJ_MIN%" == "" set /p SVN_VER_MAJ_MIN="Build Version (1.8): "
if "%SVN_VER_MAJ_MIN%" == "" set SVN_VER_MAJ_MIN=1.8

if "%SVN_VER_MAJ_MIN%" == "1.4" set SVN_VER_MAJ_DASH_MIN=1-4
if "%SVN_VER_MAJ_MIN%" == "1.5" set SVN_VER_MAJ_DASH_MIN=1-5
if "%SVN_VER_MAJ_MIN%" == "1.6" set SVN_VER_MAJ_DASH_MIN=1-6
if "%SVN_VER_MAJ_MIN%" == "1.7" set SVN_VER_MAJ_DASH_MIN=1-7
if "%SVN_VER_MAJ_MIN%" == "1.8" set SVN_VER_MAJ_DASH_MIN=1-8


rem Save CWD
pushd .

rem in development the version info can be found
rem otherwise the builder will have run it already
if "%PY_MAJ%.%PY_MIN%" == "2.4" set COMPILER=msvc71
if "%PY_MAJ%.%PY_MIN%" == "2.5" set COMPILER=msvc71
if "%PY_MAJ%.%PY_MIN%" == "2.6" set COMPILER=msvc90
if "%PY_MAJ%.%PY_MIN%" == "2.7" set COMPILER=msvc90
if "%PY_MAJ%.%PY_MIN%" == "3.0" set COMPILER=msvc90
if "%PY_MAJ%.%PY_MIN%" == "3.1" set COMPILER=msvc90
if "%PY_MAJ%.%PY_MIN%" == "3.2" set COMPILER=msvc90

if exist ..\..\ReleaseEngineering\win32-%COMPILER%\software-versions-%SVN_VER_MAJ_MIN%.cmd (
    pushd ..\..\ReleaseEngineering\win32-%COMPILER%
    call software-versions-%SVN_VER_MAJ_MIN%.cmd off
    popd
    )

set PYCXX=%WORKDIR%\Import\pycxx-%PYCXX_VER%
set PY=c:\python%PY_MAJ%%PY_MIN%.win32
if not exist %PY%\python.exe set PY=c:\python%PY_MAJ%%PY_MIN%

set PYTHONPATH=%WORKDIR%\Source
set PYTHON=%PY%\python.exe

rem Need python and SVN on the path
PATH %PY%;%SVN_BIN%;%PATH%

rem prove the python version selected
python -c "import sys;print( 'Info: Python Version %%s' %% sys.version )"
svn --version

rem restore original CWD
popd
