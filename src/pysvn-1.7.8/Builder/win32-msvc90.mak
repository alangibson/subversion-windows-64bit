BUILD_TYPE=Release
SVN_VER_MAJ_MIN=1.7

VCBUILD_OPT=/useenv /nologo /nocolor "/info:Info: " "/error:Error: " "/warning:Warn: "

build: all test kit

all:
	cd ..\Source & $(PYTHON) setup.py configure vcbuild /rebuild $(VCBUILD_OPT) pysvn-for-svn-$(SVN_VER_MAJ_DASH_MIN)-msvc90.sln "Release|Win32"


clean:
	cd ..\Source & vcbuild /clean $(VCBUILD_OPT) pysvn-for-svn-$(SVN_VER_MAJ_DASH_MIN)-msvc90.sln "Release|Win32"
	cd ..\Source & if exist sept del sept
	cd ..\Tests & $(MAKE) -f win32.mak SVN_VER_MAJ_MIN=$(SVN_VER_MAJ_MIN) clean
	cd ..\kit\Win32-$(SVN_VER_MAJ_MIN) & $(MAKE) clean

kit:
	cd ..\kit\Win32-$(SVN_VER_MAJ_MIN) & $(MAKE) all_msvc90

install:
	..\kit\Win32\tmp\output\setup.exe

test:
	cd  ..\Tests & $(MAKE) -f win32.mak SVN_VER_MAJ_MIN=$(SVN_VER_MAJ_MIN) KNOWN_GOOD_VERSION=py$(PY_MAJ)-svn$(SVN_VER_MAJ_MIN)
