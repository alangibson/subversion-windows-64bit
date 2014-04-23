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
#   make an egg of pysvn
#
import setuptools
import distutils.sysconfig

import sys
import os
import os.path
import setuptools.command.bdist_egg

pysvn_version_info = {}
f = open( 'Builder/version.info', 'r' )
for line in f:
    key, value = line.strip().split('=')
    pysvn_version_info[ key ] = value

def run(self):
    # Generate metadata first
    self.run_command("egg_info")
    os.chdir('Source')
    os.system(sys.executable + ' setup.py configure')
    os.system('make clean')
    os.system('make')
    os.system('make egg DISTDIR="%s"' % os.path.abspath(os.path.join('..', self.dist_dir)))
    os.chdir('..')              # Go back in parent directory
    # Add to 'Distribution.dist_files' so that the "upload" command works
    getattr( self.distribution, 'dist_files', [] ).append(
        ('bdist_egg', distutils.sysconfig.get_python_version(), self.egg_output) )

# Monkey patch the building method with our custom one.
setuptools.command.bdist_egg.bdist_egg.run = run

name = "pysvn"

setuptools.setup(
    name = name,
    version='%(MAJOR)s.%(MINOR)s.%(PATCH)s' % pysvn_version_info,
    author="Barry Scott",
    author_email="barryscott@tigris.org",
    description="Subversion support for Python",
    long_description="",
    url="http://pysvn.tigris.org/",
    license="Apache Software License",
    keywords="subversion",
    ext_modules = [
        setuptools.Extension(
            '_pysvn', [])       # This used to tell setuptools that
                                # there is native extension, but
                                # they're not build using setuptools.
    ],
    classifiers=[
        "Topic :: Software Development :: Version Control",
    ],
)   
