setlocal
call setup-msvc90
c:\python27.win32\python setup_makefile.py win32 win32.mak
nmake -f win32.mak clean all

c:\python32.win32\python setup_makefile.py win32 win32.mak
nmake -f win32.mak clean all

endlocal
