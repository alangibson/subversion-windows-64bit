#
#	makefile - make the windows kit
#
#	1.7 can use the 1.6 makefiles
#
all_msvc90:
	cd ..\Win32-1.6 & $(MAKE) all_msvc90
	mkdir tmp\Output
	copy /y ..\win32-1.6\tmp\Output\py*.exe tmp\Output >nul

clean:
	cd ..\Win32-1.6 & $(MAKE) clean
