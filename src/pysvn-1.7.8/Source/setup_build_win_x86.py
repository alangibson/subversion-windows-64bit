#
# ====================================================================
# (c) 2005-2009 Barry A Scott.  All rights reserved.
#
# This software is licensed as described in the file LICENSE.txt,
# which you should have received as part of this distribution.
#
# ====================================================================
#
#
#   setup.py
#
#   make it easy to build pysvn outside of svn
#
import sys, os, shutil, logging
import setup_backport
import setuptools
from setuptools import setup
from setuptools.command.install import install
import subprocess
from distutils.command.build import build
from wheel.bdist_wheel import bdist_wheel

# Save then mangle command line args to keep setuptools from complaining about unknown options
ARGV = list(sys.argv)
PYSVN_ARGS = sys.argv[3:]
sys.argv = sys.argv[0:2]
print(sys.argv)

# Generate makefiles, a la ./configure
def configure(argv):
    if setup_backport.backportRequired():
        print( 'Error: These sources are not compatible with python %d.%d - run the backport command to fix' %
            (sys.version_info[0], sys.version_info[1]) )
        return 1
            
    # must not import unless backporting has been done
    import setup_configure
    return setup_configure.cmd_configure( argv )
    
class Build(build):
    description = 'Build pysvn'

    def run(self):
    
        print('Build.build_base = ' + self.build_base)
        print(self.build_purelib)
        print(self.build_platlib)
        print(self.build_lib)
        print(self.build_temp)
        print(self.build_scripts)
        print(self.compiler)
        print(self.plat_name)
    
        configure(ARGV)
        
        output = subprocess.check_output('nmake clean', shell=True)
        print(output.decode('utf-8'))
        output = subprocess.check_output('nmake', shell=True)
        print(output.decode('utf-8'))
        
        # HACK Manually "install". The installation that bdist_wheel does does not work.
        rel_wheel_dir = self.build_base + '/bdist.'+self.plat_name+'/wheel'
        os.makedirs(rel_wheel_dir, exist_ok=True)
        shutil.copytree('pysvn', rel_wheel_dir+'/pysvn')
        
        # Doesn't work:
        # HACK Make scripts build into ./pysvn, not ./build/pysvn like they should.
        # shutil.copytree('pysvn', self.build_base+'/pysvn')

class BdistWheel(bdist_wheel):

    def run(self):
        print('bdist_dir = ' + self.bdist_dir)
        print('dist_dir = ' + self.dist_dir)
        super().run()

class Install(install):

    def run(self):
        logging.debug('Install.prefix = {}'.format(self.prefix))
        logging.debug('Install.build_base = {}'.format(self.build_base))
        logging.debug('Install.build_lib = {}'.format(self.build_lib))
        logging.debug(self.install_purelib)
        logging.debug(self.install_platlib)
        logging.debug(self.install_headers)
        logging.debug(self.install_lib)
        logging.debug(self.install_scripts)
        logging.debug(self.install_data)
        logging.debug(self.install_userbase)
        logging.debug(self.install_usersite)
        
        super().run()
        
def main( argv ):
    if argv[1:2] == ['backport']:
        if setup_backport.backportRequired():
            return setup_backport.cmd_backport( argv )
        else:
            print( 'Info: These sources are compatible with python %d.%d - no need to run the backport command' %
                (sys.version_info[0], sys.version_info[1]) )
            return 0
    elif argv[1:2] == ['configure']:
        return configure(argv)
    elif argv[1:2] == ['help']:
        setup_help( argv )
        return 0
    else:
        return setup_help( argv )

def setup_help( argv ):
    progname = os.path.basename( argv[0] )
    print( '''    Help
        python %(progname)s help

    Backport the PySVN sources to work with python 2.5 and earlier

        python %(progname)s backport
''' % {'progname': progname} )

    if setup_backport.backportRequired():
        print( '    Further help is not available until the backport command has been run.' )
        return 1

    setup_backport.cmd_help( argv )

    import setup_configure
    setup_configure.cmd_help( argv )

    return 1

setup(
    name = "pysvn",
    version = "1.7.8",
    author = "Barry Scott",
    author_email = "???",
    description = ("pysvn"),
    license = "BSD",
    keywords = "example documentation tutorial",
    url = "http://packages.python.org/an_example_pypi_project",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Topic :: Utilities",
        "License :: OSI Approved :: BSD License",
    ],
    cmdclass={
        'build': Build,
        'bdist_wheel': BdistWheel,
        'install': Install
        #'configure': configure
    },
)
