print( 'Info: setup_version_handling.py' )
import sys
sys.path.insert( 0, '..\\..\\Source')
import pysvn
import time

py_maj = sys.version_info[0]
py_min = sys.version_info[1]

python_version_string = '%d.%d.%d' % (py_maj, py_min, sys.version_info[2])
pysvn_version_string = '%d.%d.%d-%d' % (pysvn.version[0], pysvn.version[1], pysvn.version[2], pysvn.version[3])
svn_version_package_string = '%d%d%d' % (pysvn.svn_version[0], pysvn.svn_version[1], pysvn.svn_version[2])
svn_version_string = '%d.%d.%d' % (pysvn.svn_version[0], pysvn.svn_version[1], pysvn.svn_version[2])

build_time  = time.time()
build_time_str = time.strftime( '%d-%b-%Y %H:%M', time.localtime( build_time ) )

print( 'Info: Create info_before.txt' )

f = open( 'tmp\\info_before.txt', 'w' )
f.write(
'''PySVN %s for Python %s and Subversion %s

    Barry Scott

    %s

''' % (pysvn_version_string, python_version_string, svn_version_string, build_time_str) )
f.close()

print( 'Info: Creating pysvn-branded.iss from pysvn.iss' )
f = open( 'pysvn.iss', 'r' )
pysvn_iss_text = f.read()
f.close()

f = open( 'tmp\\pysvn-branded.iss', 'w' )
branding = {
        'py_maj': py_maj,
        'py_min': py_min,
        'pysvn_version_string': pysvn_version_string,
        }
print( 'Info: %r' % (branding,) )
f.write( pysvn_iss_text % branding )
f.close()

if py_maj == 3:
    msvc_system_files_iss = 'msvc90_system_files.iss'

elif py_maj == 2 and py_min >= 6:
    msvc_system_files_iss = 'msvc90_system_files.iss'

else:
    msvc_system_files_iss = 'msvc71_system_files.iss'



print( 'Info: Creating %s from pysvn.iss' % msvc_system_files_iss )
f = open( msvc_system_files_iss, 'r' )
pysvn_iss_text = f.read()
f.close()

f = open( 'tmp\\msvc_system_files.iss', 'w' )
branding = {
        'py_maj': py_maj,
        'py_min': py_min,
        'pysvn_version_string': pysvn_version_string,
        }
print( 'Info: %r' % (branding,) )
f.write( pysvn_iss_text % branding )
f.close()

print( 'Info: Create setup_copy.cmd' )
f = open( 'tmp\\setup_copy.cmd', 'w' )
f.write( 'copy tmp\\Output\\setup.exe tmp\\Output\\py%d%d-pysvn-svn%s-%s.exe\n' %
        (py_maj, py_min, svn_version_package_string, pysvn_version_string) )
f.close()
