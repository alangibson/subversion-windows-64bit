print 'Info: setup_version_handling.py'
import sys
sys.path.insert( 0, '..\\..\\Source')
import pysvn
import time

python_version_string = '%d.%d.%d' % (sys.version_info[0], sys.version_info[1], sys.version_info[2])
pysvn_version_string = '%d.%d.%d-%d' % (pysvn.version[0], pysvn.version[1], pysvn.version[2], pysvn.version[3])
svn_version_package_string = '%d%d%d' % (pysvn.svn_version[0], pysvn.svn_version[1], pysvn.svn_version[2])
svn_version_string = '%d.%d.%d' % (pysvn.svn_version[0], pysvn.svn_version[1], pysvn.svn_version[2])

build_time  = time.time()
build_time_str = time.strftime( '%d-%b-%Y %H:%M', time.localtime( build_time ) )

print 'Info: Create info_before.txt'

f = file('tmp\\info_before.txt','w')
f.write(
'''PySVN %s for Python %s and Subversion %s

    Barry Scott

    %s

''' % (pysvn_version_string, python_version_string, svn_version_string, build_time_str) )
f.close()

print 'Info: Creating pysvn-branded.iss from pysvn.iss'
f = file( 'pysvn.iss', 'r' )
pysvn_iss_text = f.read()
f.close()

f = file( 'tmp\\pysvn-branded.iss', 'w' )
branding = {
        'py_maj': sys.version_info[0],
        'py_min': sys.version_info[1],
        'pysvn_version_string': pysvn_version_string,
        }
print 'Info:',repr(branding)
f.write( pysvn_iss_text % branding )
f.close()

print 'Info: Creating msvc71_system_files.iss from pysvn.iss'
f = file( 'msvc71_system_files.iss', 'r' )
pysvn_iss_text = f.read()
f.close()

f = file( 'tmp\\msvc71_system_files.iss', 'w' )
branding = {
        'py_maj': sys.version_info[0],
        'py_min': sys.version_info[1],
        'pysvn_version_string': pysvn_version_string,
        }
print 'Info:',repr(branding)
f.write( pysvn_iss_text % branding )
f.close()

print 'Info: Create setup_copy.cmd'
f = file( 'tmp\\setup_copy.cmd', 'w' )
f.write( 'copy tmp\\Output\\setup.exe tmp\\Output\\py%d%d-pysvn-svn%s-%s.exe\n' %
        (sys.version_info[0], sys.version_info[1], svn_version_package_string, pysvn_version_string) )
f.close()
