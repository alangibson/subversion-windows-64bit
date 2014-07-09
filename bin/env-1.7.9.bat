@echo off

::
:: Environment variables to build pysvn-1.7.9
::

echo ********************************
echo ** User environment variables
echo ********************************

set PYTHON_33_64_HOME=C:\Tools\Python33_64
set PYTHON_27_32_HOME=c:\tools\Python27
:: Path is hardcoded in pysvn makefiles.
set UNXUTILS_HOME=c:\UnxUtils

set COMPILE_HOME=%CD%
:: This is the python version to build for, a.k.a. the target version for pysvn wheel
set PYTHON_HOME=PYTHON_33_64_HOME

set SVN_DIST_NAME=svn_win_x64

set APR_ICONV_VERSION=1.2.1
set APR_UTIL_VERSION=1.5.2
set APR_VERSION=1.4.8
set CYRUS_SASL_VERSION=2.1.24
set DB_VERSION=4.8.30
set EXPAT_VERSION=2.1.0
set NEON_VERSION=0.29.5
set OPENSSL_VERSION=1.0.1e
set PYCXX_VERSION=6.2.5
set PYSVN_VERSION=1.7.9
set SCONS_LOCAL_VERSION=2.3.0
set SERF_VERSION=1.2.1
:: TODO SVN probably shouldn't be named twice below
set SUBVERSION_VERSION=1.7.13
set SVN_VERSION=1.7.13
set ZLIB_VERSION=1.2.8

set TARGET_ARCH=x64
set MSVC_VERSION=9.0

echo ********************************
echo ** Set up the environment
echo ********************************

:: NOTE: UnxUtils must be expanded to c:\UnxUtils. 
:: Path is hardcoded in pysvn makefiles.
:: pysvn makefile expects touch.exe to be in non-standard location
%UNXUTILS_HOME%\usr\local\wbin\ln %UNXUTILS_HOME%\usr\local\wbin\touch.exe %UNXUTILS_HOME%\touch.exe

set DL_HOME=%COMPILE_HOME%\dl
set SOURCE_HOME=%COMPILE_HOME%\src
set BUILD_HOME=%COMPILE_HOME%\build
set DIST_HOME=%COMPILE_HOME%\dist
set SVN_DIST_DIR=%DIST_HOME%\%SVN_DIST_NAME%

:: Our 'default' python
set PYTHONDIR=%PYTHON_27_32_HOME%

:: Put python.exe, zip.exe, gzip.exe and unzip.exe on the path
set PATH=%UNXUTILS_HOME%\usr\local\wbin;%PYTHONDIR%;%PATH%
