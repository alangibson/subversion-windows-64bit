::
:: This script extracts all of the source archives and applies some fixes.
:: It should only need to be run once.
::

echo ********************************
echo ** Set environment variables
echo ********************************

env-win-x64.bat

echo ********************************
echo ** Extract source
echo ********************************

mkdir %SOURCE_HOME%
cd %SOURCE_HOME%

:: TODO Finish this
unzip.exe %DL_HOME%\apr-%APR_VERSION%-win32-src.zip 
unzip.exe %DL_HOME%\apr-iconv-%APR_ICONV_VERSION%-win32-src-r2.zip
unzip.exe %DL_HOME%\apr-util-%APR_UTIL_VERSION%-win32-src.zip
gzip.exe -dc %DL_HOME%\cyrus-sasl-%CYRUS_SASL_VERSION%*.tar.gz | tar.exe -x
unzip.exe %DL_HOME%\db-%DB_VERSION%.zip
gzip.exe -dc %DL_HOME%\expat-%EXPAT_VERSION%.tar.gz | tar.exe -x
gzip.exe -dc %DL_HOME%\pysvn-%PYSVN_VERSION%.tar.gz | tar.exe -x
gzip.exe -dc %DL_HOME%\neon-%NEON_VERSION%.tar.gz | tar.exe -x
gzip.exe -dc %DL_HOME%\openssl-%OPENSSL_VERSION%.tar.gz | tar.exe -x
gzip.exe -dc %DL_HOME%\scons-local-%SCONS_LOCAL_VERSION%.tar.gz | tar.exe -x
unzip.exe %DL_HOME%\serf-%SERF_VERSION%.zip
unzip.exe %DL_HOME%\subversion-%SUBVERSION_VERSION%.zip
:: These packages have non-standard names
unzip.exe %DL_HOME%\zlib128.zip
unzip.exe %DL_HOME%\sqlite-amalgamation-3080002.zip

echo ********************************
echo ** Apply fixes to source dir
echo ********************************

:: Convert db solution file
cd %SOURCE_HOME%\db-%DB_VERSION%\build_windows
echo Click Finish in order to convert DB solution file
devenv Berkeley_DB.sln

:: Fix APR* directory names and move under subversion directory
move %SOURCE_HOME%\apr-util-%APR_UTIL_VERSION% %SOURCE_HOME%\subversion-%SVN_VERSION%\apr-util
move %SOURCE_HOME%\apr-iconv-%APR_ICONV_VERSION% %SOURCE_HOME%\subversion-%SVN_VERSION%\apr-iconv
move %SOURCE_HOME%\apr-%APR_VERSION% %SOURCE_HOME%\subversion-%SVN_VERSION%\apr

:: Convert APR-util dsp files to vcproj/sln
echo Click Yes To All in order to convert APR-util solution file
cd %SOURCE_HOME%\subversion-%SVN_VERSION%
devenv apr-util\aprutil.dsw

:: Copy in make_dist.py that doesnt require non-required libraries
copy /y %COMPILE_HOME%\fixes\subversion-%SUBVERSION_VERSION%\build\win32\make_dist.py %SOURCE_HOME%\subversion-%SUBVERSION_VERSION%\build\win32\make_dist.py

:: Copy in more proper, bdist_wheel compatible, setup.py file
copy /y %COMPILE_HOME%\fixes\pysvn-1.7.8\Source\setup_build_win_x86.py %SOURCE_HOME%\pysvn-%PYSVN_VERSION%\Source\setup_build_win_x86.py

:: Copy all fixes into source dir
xcopy /E /Y fixes %SOURCE_HOME%\



