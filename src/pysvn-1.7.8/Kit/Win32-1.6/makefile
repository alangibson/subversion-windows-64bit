#
#	makefile - make the windows kit
#
all_msvc90: kit_msvc

kit_msvc: pysvn.iss setup_version_handling.py
	if not exist tmp mkdir tmp
	copy ..\..\LICENSE.txt tmp\LICENSE.txt
	copy $(SVN_BIN)\*.dll tmp
	$(PYTHON) setup_version_handling.py
	"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" tmp\pysvn-branded.iss
	tmp\setup_copy.cmd


debug:
	"C:\Program Files (x86)\Inno Setup 5\Compil32.exe" pysvn.iss

clean:
	if exist tmp rmdir /s /q tmp
