@echo off

::
:: From http://svn.apache.org/repos/asf/subversion/tags/1.7.13/INSTALL
:: Compiling Apache for Microsoft Windows
::	http://httpd.apache.org/docs/2.4/platform/win_compiling.html
:: Compiling APR on Windows
::	http://apr.apache.org/compiling_win32.html
:: Using Cyrus SASL Authentication with Subversion
::      http://svn.apache.org/repos/asf/subversion/trunk/notes/sasl.txt
::      https://github.com/winlibs/cyrus-sasl/blob/master/doc/windows.html
:: Build Berkley DB
::      file:///D:/dev/SVN/db-4.8.30/docs/programmer_reference/win_build64.html#id1632202
:: Building Serf
::      https://groups.google.com/forum/#!topic/subversion-development/41K7kp2kjZ4
::      http://qnalist.com/questions/4509867/error-compiling-with-serf-1-3-1-on-windows
:: Building Neon
::      http://www.opensource.apple.com/source/neon/neon-11/neon/INSTALL.win32
::      http://www.apachehaus.com/forum/index.php?topic=143.50;wap2
:: Building pysvn Extension
::	http://pysvn.tigris.org/source/browse/*checkout*/pysvn/tags/pysvn/Extension/1.7.8/INSTALL.html
::

echo ********************************
echo ** Set environment variables
echo ********************************

call env-1.7.9.bat
:: call this here because it doesnt work from inside env*.bat
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" %TARGET_ARCH%

echo ********************************
echo ** Clean up 
echo ********************************

call clean.bat

echo ********************************
echo ** Version check build tools
echo ********************************

python -V
perl -v
awk
nmake
devenv /?

echo ********************************
echo ** Build OpenSSL
echo ********************************

cd %BUILD_HOME%\openssl-%OPENSSL_VERSION%

perl Configure VC-WIN64A

call ms\do_win64a

nmake -f ms\ntdll.mak

cd out32dll

call ..\ms\test

cd %BUILD_HOME%

echo ********************************
echo ** Build Berkley DB
echo ********************************

cd %BUILD_HOME%\db-%DB_VERSION%\build_windows

:: Convert solution file
:: echo Click Finish in order to convert solution file
:: devenv Berkeley_DB.sln

devenv Berkeley_DB.sln /build "Release|x64"

cd %BUILD_HOME%

echo ********************************
echo ** Build SASL
echo ********************************

cd %BUILD_HOME%\cyrus-sasl-%CYRUS_SASL_VERSION%

nmake /f NTMakefile CFG=Release ^
  OPENSSL_INCLUDE=%BUILD_HOME%\openssl-%OPENSSL_VERSION%\include ^
  OPENSSL_LIBPATH=%BUILD_HOME%\openssl-%OPENSSL_VERSION%\out32dll ^
  DB_INCLUDE=%BUILD_HOME%\db-%DB_VERSION%\build_windows ^
  DB_LIBPATH=%BUILD_HOME%\db-%DB_VERSION%\build_windows\x64\Release ^
  DB_LIB=libdb48.lib

cd %BUILD_HOME%

echo ********************************
echo ** Build APR
echo ********************************

cd %BUILD_HOME%\subversion-%SVN_VERSION%

cd apr-util

devenv aprutil.sln /project "libapr" /build "Release|x64"
devenv aprutil.sln /project "libapriconv" /build "Release|x64"
devenv aprutil.sln /project "xml" /build "Release|x64"
devenv aprutil.sln /project "libaprutil" /build "Release|x64"

cd ..

:: Log shows /libdir does not include \x64\
rmdir /Q /S apr\Release
move apr\x64\Release apr\Release

rmdir /Q /S apr-util\Release
move apr-util\x64\Release apr-util\Release

rmdir /Q /S apr-iconv\Release
move apr-iconv\x64\Release apr-iconv\Release

rmdir /Q /S apr-util\xml\expat\lib\LibR
move apr-util\xml\expat\lib\x64\LibR apr-util\xml\expat\lib\LibR

cd %BUILD_HOME%

echo ********************************
echo ** Build zlib
echo ********************************

cd %BUILD_HOME%\zlib-%ZLIB_VERSION%

nmake /f win32\Makefile.msc clean

nmake /f win32\Makefile.msc

cd %BUILD_HOME%

echo ********************************
echo ** Build Subversion
echo ********************************

cd %BUILD_HOME%\subversion-%SUBVERSION_VERSION%

:: HACK Forcing serf-1.2.1 since Subversion wants serf.mak file
::set ORIG_SERF_VERSION=%SERF_VERSION%
::set SERF_VERSION=1.2.1

%PYTHON_27_32_HOME%\python gen-make.py --vsnet-version=2008 -t vcproj ^
  --with-openssl=%BUILD_HOME%\openssl-%OPENSSL_VERSION% ^
  --with-zlib=%BUILD_HOME%\zlib-%ZLIB_VERSION% ^
  --without-neon ^
  --with-serf=%BUILD_HOME%\serf-%SERF_VERSION% ^
  --with-sasl=%BUILD_HOME%\cyrus-sasl-%CYRUS_SASL_VERSION% ^
  --with-sqlite=%BUILD_HOME%\sqlite-amalgamation-3080002

:: Rebuild is called first because incremental linking can cause
:: problems when you have multiple copies of Visual Studio installed
devenv subversion_vcnet.sln /rebuild "Release|x64" /project "__ALL_TESTS__"
devenv subversion_vcnet.sln /build "Release|x64" /project "__ALL_TESTS__"
:: Run once more because a 'clean' build will still have a few errors
devenv subversion_vcnet.sln /build "Release|x64" /project "__ALL_TESTS__"

:: Make the Subversion distribution
mkdir %DIST_HOME%
copy /y build\win32\make_dist.conf.template build\win32\make_dist.conf
rmdir /Q /S %SVN_DIST_DIR%
%PYTHON_27_32_HOME%\python build/win32/make_dist.py %SVN_DIST_NAME% %DIST_HOME%
:: Note: lots of useful debugging info is written to: build\win32\make_dist.log

cd %BUILD_HOME%

echo ********************************
echo ** Test Subversion
echo ********************************

cd %SVN_DIST_DIR%\bin
svn.exe --version

echo ********************************
echo ** Build pysvn
echo ********************************

cd %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Builder

:: Building for python 3.3, so change path
set ORIGPATH=%PATH%
set PATH=%PYTHON_33_64_HOME%;%ORIGPATH%

:: builder-custom_init.cmd also uses an SVN_BIN variable
set SVN_BIN=%SVN_DIST_DIR%\bin

:: Do we need "3_64" because the python dir is Python33_64?
:: builder_custom_init.cmd 2 7 ? 1.7
call builder_custom_init.cmd 3 3_64 ? 1.7

:: reset some vars for our environment
set PY=%PYTHON_33_64_HOME%
set PYTHON=%PY%\python.exe

:: setup_configure.py expects serf-1.lib to be at %(SVN_LIB)s\serf\serf-1.lib
:: Manually copy serf-1.lib since it is not copied by the subversion release process
:: make_dist.log makes it seem like serf.lib, not serf-1.lib, is copied (?)
mkdir %SVN_DIST_DIR%\lib\serf\
copy /y %BUILD_HOME%\serf-%SERF_VERSION%\Release\serf-1.lib %SVN_DIST_DIR%\lib\serf\

cd %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source 

%PYTHON_33_64_HOME%\python.exe setup.py configure ^
  --platform=win32 ^
  --pycxx-dir=..\Import\pycxx-%PYCXX_VERSION% ^
  --svn-inc-dir=%SVN_DIST_DIR%\include ^
  --svn-lib-dir=%SVN_DIST_DIR%\lib ^
  --svn-bin-dir=%SVN_DIST_DIR%\bin ^
  --apr-inc-dir=%SVN_DIST_DIR%\include\apr ^
  --apu-inc-dir=%SVN_DIST_DIR%\include\apr-util ^
  --apr-lib-dir=%SVN_DIST_DIR%\lib\apr

nmake clean
nmake

cd ..\Tests
nmake clean
nmake

:: TODO Getting error: NMAKE : fatal error U1077: '"C:\Program Files (x86)\Inno Setup 5\ISCC.exe' : return code '0x1'
:: cd ..\Kit\Win32-1.7
:: nmake clean
:: nmake

echo ********************************
echo ** Build pysvn wheel
echo ********************************

:: Requires pip wheel support: http://wheel.readthedocs.org/en/latest/#usage

cd %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source

:: Make sure there are no pyc files in the source dir
del pysvn\__init__.pyc
rmdir /q /s pysvn\__pycache__

%PYTHON_33_64_HOME%\python.exe setup_build_win_x86.py bdist_wheel ^
  --platform=win32 ^
  --pycxx-dir=..\Import\pycxx-%PYCXX_VERSION% ^
  --svn-inc-dir=%SVN_DIST_DIR%\include ^
  --svn-lib-dir=%SVN_DIST_DIR%\lib ^
  --svn-bin-dir=%SVN_DIST_DIR%\bin ^
  --apr-inc-dir=%SVN_DIST_DIR%\include\apr ^
  --apu-inc-dir=%SVN_DIST_DIR%\include\apr-util ^
  --apr-lib-dir=%SVN_DIST_DIR%\lib\apr
  
echo ********************************
echo ** Fix wheel
echo ********************************

cd %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source\dist

:: Expand wheel
unzip.exe -o pysvn-%PYSVN_VERSION%-*.whl
rm pysvn-%PYSVN_VERSION%-*.whl

:: Remove useless files
cd pysvn
del *.template
del *.ilk
del *.lib
del *.pdb
del *.exp

:: Add in missing files
copy %SVN_DIST_DIR%\bin\*.dll 

:: Zip wheel back up
cd ..
zip -r pysvn-%PYSVN_VERSION%-cp33-none-win_amd64.whl * 

:: Clean up previously expanded files
rmdir /q /s pysvn
rmdir /q /s pysvn-%PYSVN_VERSION%.dist-info

cd %BUILD_HOME%

echo ********************************
echo ** Test pysvn
echo ********************************

cd %BUILD_HOME%\pysvn-%PYSVN_VERSION%

:: PYTHONPATH somehow gets set to %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source, so remove it
set PYTHONPATH=

:: Manual installation. Only for debugging.
:: xcopy /E %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source\pysvn %PYTHON_33_64_HOME%\Lib\site-packages\pysvn\

:: (Re)install pysvn with pip
%PYTHON_33_64_HOME%\Scripts\pip.exe uninstall -y pysvn
%PYTHON_33_64_HOME%\Scripts\pip.exe install --use-wheel %BUILD_HOME%\pysvn-%PYSVN_VERSION%\Source\dist\pysvn-%PYSVN_VERSION%-cp33-none-win_amd64.whl

:: Test installation
%PYTHON_33_64_HOME%\python.exe -c "import pysvn; print(pysvn.version)"
