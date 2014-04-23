#
#	makefile - make the windows kit
#
all_msvc71: kit_msvc

all_msvc90: kit_msvc

kit_msvc: pysvn-branded.iss info_before.txt copy_setup.cmd
	copy ..\..\LICENSE.txt tmp\LICENSE.txt
	copy $(SUBVERSION)\svn-win32-libintl\bin\intl3_svn.dll tmp
	copy $(SUBVERSION)\db4-win\bin\libdb44.dll tmp
	copy $(OPENSSL)\out32dll\libeay32.dll tmp
	copy $(OPENSSL)\out32dll\ssleay32.dll tmp
	copy $(SUBVERSION)\Release\bin\libapr-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libaprutil-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libapriconv-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libexpat.dll tmp
	copy $(SUBVERSION)\Release\bin\libgettextlib.dll tmp
	copy $(SUBVERSION)\Release\bin\libgettextpo.dll tmp
	copy $(SUBVERSION)\Release\bin\libgettextsrc.dll tmp
	copy $(SUBVERSION)\Release\bin\libiconv2.dll tmp
	copy $(SUBVERSION)\Release\bin\libintl3.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_client-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_delta-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_diff-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_fs-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_ra-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_repos-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_subr-1.dll tmp
	copy $(SUBVERSION)\Release\bin\libsvn_wc-1.dll tmp
	"c:\Program Files\Inno Setup 5\ISCC.exe" tmp\pysvn-branded.iss
	tmp\setup_copy.cmd


info_before.txt: pysvn-branded.iss

copy_setup.cmd: pysvn-branded.iss

pysvn-branded.iss: setup_version_handling.py pysvn.iss
	if not exist tmp mkdir tmp
	python setup_version_handling.py

debug:
	"c:\Program Files\Inno Setup 5\Compil32.exe" pysvn.iss

clean:
	if exist tmp rmdir /s /q tmp
