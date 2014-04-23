#
# ====================================================================
# Copyright (c) 2005-2011 Barry A Scott.  All rights reserved.
#
# This software is licensed as described in the file LICENSE.txt,
# which you should have received as part of this distribution.
#
# ====================================================================
#
#
#   setup_configure.py
#
#   make it easy to build pysvn outside of svn
#
import sys
import os
import distutils
import distutils.sysconfig
import distutils.util

import xml.dom.minidom
import xml.sax

class SetupError(Exception):
    pass

# version of PyCXX that we require
pycxx_version = (6, 2, 4)

_debug = False

def debug( msg ):
    if _debug:
        sys.stderr.write( 'Debug: %s\n' % (msg,) )

def cmd_configure( argv ):
    try:
        o = Options( argv )
        setup = Setup( o )

        setup.generateSourcesMakefile()
        setup.generateTestMakefile()
        return 0

    except SetupError as e:
        print( 'Error:', str(e) )
        return 1

def cmd_help( argv ):
    o = Options( argv )
    o.usage()
    return 1

class Options:
    all_options_info = {
        '--arch':               (2, '<arch>'),
        '--apr-inc-dir':        (1, '<dir>'),
        '--apu-inc-dir':        (1, '<dir>'),
        '--apr-lib-dir':        (1, '<dir>'),
        '--debug':              (0, None),  # debug this script
        '--define':             (2, '<define-string>'),
        '--enable-debug':       (0, None),
        '--fixed-module-name':  (0, None),
        '--norpath':            (0, None),
        '--platform':           (1, '<platform-name>'),
        '--pycxx-dir':          (1, '<dir>'),
        '--pycxx-src-dir':      (1, '<dir>'),
        '--svn-inc-dir':        (1, '<dir>'),
        '--svn-lib-dir':        (1, '<dir>'),
        '--svn-bin-dir':        (1, '<dir>'),
        '--svn-root-dir':       (1, '<dir>'),
        '--verbose':            (0, None),
        }

    def __init__( self, argv ):
        self.__argv = argv
        self.__progname = os.path.basename( argv[0] )
        self.__all_options = {}
        self.__used_options = set()

    def usage( self ):
        print( '''    Create a makefile for this python and platform

            python %(progname)s configure <options>

        where <options> is one or more of:
''' % {'progname': self.__progname} )
        for option, num_value in sorted( self.all_options_info.items() ):
            num, value = num_value
            if num == 0:
                print( '        %s' % (option,) )
            else:
                print( '        %s=%s' % (option,value) )

    def parseOptions( self ):
        for option in self.__argv[2:]:
            option_parts = option.split( '=', 1 )
            option_name = option_parts[0]
            if option_name not in self.all_options_info:
                print( 'Error: Unknown option %s' % option )
                return False

            repeat_count, value = self.all_options_info[ option_name ]
            if repeat_count == 0:
                if len(option_parts) != 1:
                    print( 'Error: Option %s does not take a value' % (option_name,) )
                    return False

                self.__all_options[ option_name ] = None

            elif repeat_count == 1:
                if len(option_parts) != 2:
                    print( 'Error: Option %s requires a value' % (option_name,) )
                    return False

                if option_name in self.__all_options:
                    print( 'Error: only one %s is allowed' % (option_name,) )
                    return False

                self.__all_options[ option_name ] = option_parts[1]

            elif repeat_count == 2:
                if len(option_parts) != 2:
                    print( 'Error: Option %s requires a value' % (option_name,) )
                    return False

                self.__all_options.setdefault( option_name, [] ).append( option_parts[1] )

        global _debug
        if self.hasOption( '--debug' ):
            _debug = True

        return True

    def hasOption( self, option_name ):
        self.__used_options.add( option_name )
        return option_name in self.__all_options

    def getOption( self, option_name ):
        self.__used_options.add( option_name )
        return self.__all_options[ option_name ]

    def checkAllOptionsUsed( self ):
        all_options = set( self.__all_options )
        unused_options = all_options - self.__used_options
        if len(unused_options) > 0:
            print( 'Error: Unused options: %s' % (', '.join( unused_options ),) )
            return False

        return True

#--------------------------------------------------------------------------------
class Setup:
    def __init__( self, options ):
        self.options = options

    def makePrint( self, line ):
        self.__makefile.write( line )
        self.__makefile.write( '\n' )

    def setupCompile( self ):
        print( 'Info: Configure for python %d.%d.%d in exec_prefix %s' %
                (sys.version_info[0], sys.version_info[1], sys.version_info[2]
                ,sys.exec_prefix) )

        if self.options.hasOption( '--platform' ):
            self.platform = self.options.getOption( '--platform' )

        else:
            if sys.platform == 'darwin' and os.path.exists( '/System/Library/CoreServices/SystemVersion.plist' ):
                self.platform = 'macosx'

            elif sys.platform.startswith('aix'):
                self.platform = 'aix'

            elif sys.platform.startswith('sunos5'):
                self.platform = 'sunos5'

            elif sys.platform.startswith('linux'):
                self.platform = 'linux'

            elif sys.platform.startswith('freebsd'):
                self.platform = 'freebsd'

            elif sys.platform == 'cygwin':
                self.platform = 'cygwin'

            else:
                raise SetupError( 'Cannot automatically detect your platform use --platform option' )

        if self.platform == 'win32':
            self.c_utils = Win32CompilerMSVC90( self )
            self.c_pysvn = Win32CompilerMSVC90( self )

        elif self.platform == 'macosx':
            self.c_utils = MacOsxCompilerGCC( self )
            self.c_pysvn = MacOsxCompilerGCC( self )

        elif self.platform == 'linux':
            self.c_utils = LinuxCompilerGCC( self )
            self.c_pysvn = LinuxCompilerGCC( self )

        elif self.platform == 'freebsd':
            self.c_utils = FreeBsdCompilerGCC( self )
            self.c_pysvn = FreeBsdCompilerGCC( self )

        elif self.platform == 'cygwin':
            self.c_utils = CygwinCompilerGCC( self )
            self.c_pysvn = CygwinCompilerGCC( self )

        elif self.platform == 'aix':
            self.c_utils = AixCompilerGCC( self )
            self.c_pysvn = AixCompilerGCC( self )

        elif self.platform == 'sunos5':
            self.c_utils = SunOsCompilerGCC( self )
            self.c_pysvn = SunOsCompilerGCC( self )

        else:
            raise SetupError( 'Unknown platform %r' % (self.platform,) )

        print( 'Info: Using tool chain %s' % (self.c_utils.__class__.__name__,) )

        self.c_utils.setupUtilities()
        self.c_pysvn.setupPySvn()

        self.pycxx_obj_file = [
            Source( self.c_pysvn, '%(PYCXX_SRC)s/cxxsupport.cxx' ),
            Source( self.c_pysvn, '%(PYCXX_SRC)s/cxx_extensions.cxx' ),
            Source( self.c_pysvn, '%(PYCXX_SRC)s/cxxextensions.c' ),
            Source( self.c_pysvn, '%(PYCXX_SRC)s/IndirectPythonInterface.cxx' ),
            ]

        pysvn_headers = ['pysvn.hpp', 'pysvn_docs.hpp', 'pysvn_svnenv.hpp', 'pysvn_static_strings.hpp', 'pysvn_version.hpp']
        self.pysvn_obj_files = [
            Source( self.c_pysvn, 'pysvn.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_callbacks.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_static_strings.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_enum_string.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_add.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_changelist.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_checkin.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_copy.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_diff.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_export.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_info.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_list.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_lock.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_merge.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_prop.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_revprop.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_client_cmd_switch.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_transaction.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_revision.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_docs.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_path.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_arg_processing.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_converters.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_svnenv.cpp', pysvn_headers ),
            Source( self.c_pysvn, 'pysvn_profile.cpp', pysvn_headers ),
            ] + self.pycxx_obj_file

        self.generate_svn_error_codes_obj_files = [
            Source( self.c_utils, 'generate_svn_error_codes/generate_svn_error_codes.cpp', ['generate_svn_error_codes.hpp'] ),
            ]

        self.all_support_targets = [
            PysvnDocsSource( self.c_pysvn ),
            PysvnVersionHeader( self.c_pysvn ),
            GenerateSvnErrorCodesHeader( self.c_pysvn ),
            Program( self.c_utils, 'generate_svn_error_codes/generate_svn_error_codes', self.generate_svn_error_codes_obj_files ),
            PysvnDocsHeader( self.c_pysvn, ['pysvn_docs.cpp'] )
            ]

        self.all_exe = [
            PysvnModuleInit( self.c_pysvn ),
            PythonExtension( self.c_pysvn, self.c_pysvn.expand( 'pysvn/%(PYSVN_MODULE_BASENAME)s' ), self.pysvn_obj_files ),
            ]

    def generateSourcesMakefile( self ):
        if not self.options.parseOptions():
            raise SetupError( 'failed to parse options' )

        self.__makefile = open( 'Makefile', 'wt' )

        self.setupCompile()

        print( 'Info: Creating Makefile for Sources' )
        self.c_pysvn.generateMakefileHeader()

        self.makePrint( 'all: %s' % (' '.join( [exe.getTargetFilename() for exe in self.all_exe] )) )
        self.makePrint( '' )

        for exe in self.all_exe:
            exe.generateMakefile()

        for target in self.all_support_targets:
            target.generateMakefile()

        self.__makefile.close()
        self.__makefile = None

        if not self.options.checkAllOptionsUsed():
            raise SetupError( 'not all options used' )

        return 0

    def generateTestMakefile( self ):
        print( 'Info: Creating Makefile for Tests' )

        all_normal_test_cases = [
            TestCase( self.c_pysvn, '01' ),
            TestCase( self.c_pysvn, '04' ),
            TestCase( self.c_pysvn, '05' ),
            TestCase( self.c_pysvn, '06' ),
            TestCase( self.c_pysvn, '07' ),
            ]

        all_extra_test_cases = [
            TestCase( self.c_pysvn, '03' ),
            ]

        all_test_cases = []
        all_test_cases.extend( all_normal_test_cases )
        all_test_cases.extend( all_extra_test_cases )

        self.__makefile = open( '../Tests/Makefile', 'wt' )

        self.c_pysvn.ruleAllTestCase( 'all', all_normal_test_cases )
        self.c_pysvn.ruleAllTestCase( 'extratests', all_extra_test_cases )

        self.makePrint( 'clean: %s' % (' '.join( ['clean-%s' % (tc.test_name,) for tc in all_test_cases] )) )

        for test_case in all_test_cases:
            test_case.generateMakefile()

        self.__makefile.close()
        self.__makefile = None

        return 0

#--------------------------------------------------------------------------------
class Compiler:
    def __init__( self, setup ):
        debug( 'Compiler.__init__()' )
        self.setup = setup
        self.options = setup.options

        self.verbose = self.options.hasOption( '--verbose' )

        self.__variables = {}

        self._addVar( 'PYCXX_VER',       '6.2.4' )
        self._addVar( 'DEBUG',           'NDEBUG')    

        self._addVar( 'PYSVN_SRC_DIR', os.path.dirname( os.getcwd() ) )

        self.py_module_defines = []
        self.py_module_init_function = None

    def _getDefines( self, arg_fmt ):
        all_defines = []
        if self.setup.options.hasOption( '--define' ):
            for define in self.setup.options.getOption( '--define' ):
                all_defines.append( arg_fmt % (define,) )

        return all_defines

    def _pysvnModuleSetup( self ):
        if self.options.hasOption( '--fixed-module-name' ):
            print( 'Info: Using fixed module name' )
            self.pysvn_module_name = '_pysvn'
            if sys.version_info[0] >= 3:
                self.py_module_init_function = 'PyInit__pysvn'
            else:
                self.py_module_init_function = 'init_pysvn'

        else:
            # name of the module including the python version to help
            # ensure that only a matching _pysvn.so for the version of
            # python is imported

            self.pysvn_module_name = '_pysvn_%d_%d' % (sys.version_info[0], sys.version_info[1])
            if sys.version_info[0] >= 3:
                self.py_module_init_function = 'PyInit__pysvn_%d_%d' % sys.version_info[:2]
                self.py_module_defines.append( ('PyInit__pysvn', self.py_module_init_function) )
                self.py_module_defines.append( ('PyInit__pysvn_d', '%s_d' % (self.py_module_init_function,)) )
            else:
                self.py_module_init_function = 'init_pysvn_%d_%d' % sys.version_info[:2]
                self.py_module_defines.append( ('init_pysvn', self.py_module_init_function) )
                self.py_module_defines.append( ('init_pysvn_d', '%s_d' % (self.py_module_init_function,)) )


    def _completeInit( self ):
        # must be called  by the leaf derived class to finish initialising the compile

        self._addVar( 'PYTHON',         sys.executable )

        self._addVar( 'PYCXX',          self.find_pycxx() )
        self._addVar( 'PYCXX_SRC',      self.find_pycxx_src() )
        svn_inc = self.find_svn_inc()
        self._addVar( 'SVN_INC',        svn_inc )
        self._addVar( 'SVN_LIB',        self.find_svn_lib() )
        self._addVar( 'SVN_BIN',        self.find_svn_bin() )
        self._addVar( 'APR_INC',        self.find_apr_inc() )
        self._addVar( 'APU_INC',        self.find_apu_inc() )
        self._addVar( 'APR_LIB',        self.find_apr_lib() )

        self.__getSvnVersion( svn_inc )

    def platformFilename( self, filename ):
        return filename

    def makePrint( self, line ):
        self.setup.makePrint( line )

    def generateMakefileHeader( self ):
        raise NotImplementedError( 'generateMakefileHeader' )

    def _addFromEnv( self, name ):
        debug( 'Compiler._addFromEnv( %r )' % (name,) )

        self._addVar( name, os.environ[ name ] )

    def _addVar( self, name, value ):
        debug( '%s._addVar( %r, %r )' % (self.__class__.__name__, name, value) )

        try:
            if '%' in value:
                value = value % self.__variables

            self.__variables[ name ] = value

        except TypeError:
            raise SetupError( 'Cannot addVar name %r value %r' % (name, value) )

        except KeyError as e:
            raise SetupError( 'Cannot addVar name %r value %r - %s' % (name, value, e) )

    def expand( self, s ):
        try:
            return s % self.__variables

        except (TypeError, KeyError) as e:
            print( 'Error: %s' % (e,) )
            print( 'String: %s' % (s,) )
            for key, value in sorted( self.__variables.items() ):
                print( 'Variable: %s: %r' % (key, value) )

            raise SetupError( 'Cannot expand string %r - %s' % (s,e) )


    def find_pycxx( self ):
        pycxx_dir = self.find_dir(
                    'PyCXX include',
                    '--pycxx-dir',
                    None,
                    self._find_paths_pycxx_dir,
                    'CXX/Version.hxx' )

        major_match = False
        minor_match = False
        f = open( os.path.join( pycxx_dir, 'CXX/Version.hxx' ) )
        for line in f:
            words = line.split()
            if 'PYCXX_VERSION_MAJOR' in words:
                major_match = int( words[2] ) == pycxx_version[0]
            if 'PYCXX_VERSION_MINOR' in words:
                minor_match = int( words[2] ) == pycxx_version[1]

        if not major_match and not minor_match:
            raise SetupError( 'PyCXX version not as required.' )

        return pycxx_dir

    def find_pycxx_src( self ):
        v = {'PYCXX': self.find_pycxx()}

        return self.find_dir(
                    'PyCXX Source',
                    '--pycxx-src-dir',
                    None,
                    [p % v for p in self._find_paths_pycxx_src],
                    'cxxsupport.cxx' )

    def find_svn_inc( self ):
        for folder in ['include/subversion-1', 'include']:
            try:
                return self.find_dir(
                            'SVN include',
                            '--svn-inc-dir',
                            folder,
                            self._find_paths_svn_inc,
                            'svn_client.h' )
            except SetupError as e:
                last_exception = e

        raise last_exception

    def find_svn_bin( self ):
        return self.find_dir(
                    'SVN bin',
                    '--svn-bin-dir',
                    'bin',
                    self._find_paths_svn_bin,
                    'svnadmin%s' % (self.getProgramExt(),) )

    def find_svn_lib( self ):
        folder = self.find_dir(
                    'SVN library',
                    '--svn-lib-dir',
                    'lib',
                    self._find_paths_svn_lib,
                    self.get_lib_name_for_platform( 'libsvn_client-1' ) )
        # if we are using the Fink SVN then remember this
        self.is_mac_os_x_fink = folder.startswith( '/sw/' )
        self.is_mac_os_x_darwin_ports = folder.startswith( '/opt/local/' )
        return folder

    def find_apr_inc( self ):
        return self.find_dir(
            'APR include',
            '--apr-inc-dir',
            None,
            self._find_paths_apr_inc,
            'apr.h' )

    def find_apu_inc( self ):
        return self.find_dir(
            'APR include',
            '--apu-inc-dir',
            None,
            self._find_paths_apr_util_inc,
            'apu.h' )

    def find_apr_lib( self ):
        last_exception = None
        lib_list = [(self.get_lib_name_for_platform( 'libapr-1' ), 'apr-1'),
                    (self.get_lib_name_for_platform( 'libapr-0' ), 'apr-0')]

        for apr_libname, self.lib_apr in lib_list:
            try:
                return self.find_dir(
                    'APR library',
                    '--apr-lib-dir',
                    'lib',
                    [],
                    apr_libname )
            except SetupError:
                pass

        for apr_libname, self.lib_apr in lib_list:
            try:
                return self.find_dir(
                    'APR library',
                    '--apr-lib-dir',
                    None,
                    self._find_paths_apr_lib,
                    apr_libname )
            except SetupError as e:
                last_exception = e
        raise last_exception

    def get_lib_name_for_platform( self, libname ):
        raise NotImplementedError

    def find_dir( self, name, kw, svn_root_suffix, base_dir_list, check_file ):
        dirname = self.__find_dir( name, kw, svn_root_suffix, base_dir_list, check_file )
        print( 'Info: Found %14.14s in %s' % (name, dirname) )
        return dirname

    def __find_dir( self, name, kw, svn_root_suffix, base_dir_list, check_file ):
        debug( '__find_dir( name=%r, kw=%r, svn_root_suffix=%r, base_dir_list=%r, check_file=%r )' %
                    (name, kw, svn_root_suffix, base_dir_list, check_file) )

        # override the base_dir_list from the command line kw
        svn_root_dir = None

        if self.options.hasOption( kw ):
            base_dir_list = [self.options.getOption( kw )]

        elif( self.options.hasOption( '--svn-root-dir' )
        and svn_root_suffix is not None ):
            base_dir_list = ['%s/%s' % (self.options.getOption( '--svn-root-dir' ), svn_root_suffix)]

        debug( '__find_dir base_dir_list=%r' % (base_dir_list,) )

        # expect to find check_file in one of the dir
        for dirname in base_dir_list:
            full_check_file = os.path.join( dirname, check_file )
            if self.verbose:
                print( 'Info: Checking for %s in %s' % (name, full_check_file) )
            if os.path.exists( full_check_file ):
                return os.path.abspath( dirname )

        raise SetupError( 'cannot find %s %s - use %s' % (name, check_file, kw) )

    def __getSvnVersion( self, svn_include ):
        all_svn_version_lines = open( os.path.join( svn_include, 'svn_version.h' ) ).readlines()
        major = None
        minor = None
        patch = None
        for line in all_svn_version_lines:
            words = line.strip().split()
            if len(words) > 2 and words[0] == '#define':
                if words[1] == 'SVN_VER_MAJOR':
                    major = int(words[2])
                elif words[1] == 'SVN_VER_MINOR':
                    minor = int(words[2])
                elif words[1] == 'SVN_VER_PATCH':
                    patch = int(words[2])
 
        print( 'Info: Building against SVN %d.%d.%d' % (major, minor, patch) )
        self.__svn_version_tuple = (major, minor, patch)

    def getSvnVersion( self ):
        return self.__svn_version_tuple


class Win32CompilerMSVC90(Compiler):
    def __init__( self, setup ):
        Compiler.__init__( self, setup )

        self._find_paths_pycxx_dir = []
        self._find_paths_pycxx_src = [
                        '%(PYCXX)s/Src',
                        ]
        self._find_paths_svn_inc = []
        self._find_paths_svn_bin = []
        self._find_paths_svn_lib = []
        self._find_paths_apr_inc = []
        self._find_paths_apr_util_inc = []
        self._find_paths_apr_lib = []

        if self.options.hasOption( '--svn-root-dir' ):
            svn_root_dir = self.options.getOption( '--svn-root-dir' )
            self._find_paths_apr_inc.append( r'%s\include\apr' % (svn_root_dir,) )
            self._find_paths_apr_util_inc.append( r'%s\include\apr-util' % (svn_root_dir,) )
            self._find_paths_apr_lib.append( r'%s\lib\apr' % (svn_root_dir,) )

        self._addVar( 'PYTHON_DIR',     sys.exec_prefix )
        self._addVar( 'PYTHON_INC',     r'%(PYTHON_DIR)s\include' )
        self._addVar( 'PYTHON_LIB',     r'%(PYTHON_DIR)s\libs' )

        self._completeInit()

    def platformFilename( self, filename ):
        return filename.replace( '/', '\\' )

    def getPythonExtensionFileExt( self ):
        return '.pyd'

    def getProgramExt( self ):
        return '.exe'

    def getObjectExt( self ):
        return '.obj'

    def getTouchCommand( self ):
        return r'c:\UnxUtils\touch'

    def generateMakefileHeader( self ):
        self.makePrint( '#' )
        self.makePrint( '#        Pysvn Makefile generated by setup.py' )
        self.makePrint( '#' )
        self.makePrint( 'CCC=cl /nologo' )
        self.makePrint( 'CC=cl /nologo' )
        self.makePrint( '' )
        self.makePrint( 'LDSHARED=$(CCC) /LD /Zi /MT /EHsc' )
        self.makePrint( 'LDEXE=$(CCC) /Zi /MT /EHsc' )
        self.makePrint( self.expand( 'LDLIBS=%(LDLIBS)s' ) )
        self.makePrint( '' )

    def ruleLinkProgram( self, target ):
        pyd_filename = target.getTargetFilename()
        pdf_filename = target.getTargetFilename( '.pdf' )

        all_objects = [source.getTargetFilename() for source in target.all_sources]

        rules = ['']

        rules.append( '' )
        rules.append( '%s : %s' % (pyd_filename, ' '.join( all_objects )) )
        rules.append( '\t@echo Link %s' % (pyd_filename,) )
        rules.append( '\t@$(LDEXE)  %%(CCCFLAGS)s /Fe%s /Fd%s %s $(LDLIBS)' %
                            (pyd_filename, pdf_filename, ' '.join( all_objects )) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleLinkShared( self, target ):
        pyd_filename = target.getTargetFilename()
        pdf_filename = target.getTargetFilename( '.pdf' )

        all_objects = [source.getTargetFilename() for source in target.all_sources]

        rules = ['']

        rules.append( '' )
        rules.append( '%s : %s' % (pyd_filename, ' '.join( all_objects )) )
        rules.append( '\t@echo Link %s' % (pyd_filename,) )
        rules.append( '\t@$(LDSHARED)  %%(CCCFLAGS)s /Fe%s /Fd%s %s %%(PYTHON_LIB)s\python%d%d.lib $(LDLIBS)' %
                            (pyd_filename, pdf_filename, ' '.join( all_objects ), sys.version_info[0], sys.version_info[1]) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleCxx( self, target ):
        obj_filename = target.getTargetFilename()

        rules = []

        rules.append( '%s: %s %s' % (obj_filename, target.src_filename, ' '.join( target.all_dependencies )) )
        rules.append( '\t@echo Compile: %s into %s' % (target.src_filename, target.getTargetFilename()) )
        rules.append( '\t@$(CCC) /c %%(CCCFLAGS)s /Fo%s /Fd%s %s' % (obj_filename, target.dependent.getTargetFilename( '.pdb' ), target.src_filename) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleC( self, target ):
        # can reuse the C++ rule
        self.ruleCxx( target )

    def ruleClean( self, filename ):
        rules = []
        rules.append( 'clean::' )
        rules.append( '\tif exist %s del %s' % (filename, filename) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def get_lib_name_for_platform( self, libname ):
        return '%s.lib' % (libname,)

    def setupUtilities( self ):
        self._addVar( 'CCCFLAGS',
                                        r'/Zi /MT /EHsc '
                                        r'/I. /I%(APR_INC)s /I%(APU_INC)s /I%(SVN_INC)s '
                                        r'/D_CRT_NONSTDC_NO_DEPRECATE '
                                        r'/U_DEBUG '
                                        r'/DWIN32 '
                                        r'/D%(DEBUG)s' )

    def setupPySvn( self ):
        self._pysvnModuleSetup()
        self._addVar( 'PYSVN_MODULE_BASENAME', self.pysvn_module_name )


        py_cflags_list = [
                                        r'/Zi /MT /EHsc ',
                                        r'/I. /I%(APR_INC)s /I%(APU_INC)s /I%(SVN_INC)s',
                                        r'/DPYCXX_PYTHON_2TO3 /I%(PYCXX)s /I%(PYCXX_SRC)s /I%(PYTHON_INC)s',
                                        r'/D_CRT_NONSTDC_NO_DEPRECATE',
                                        r'/U_DEBUG',
                                        r'/DWIN32',
                                        r'/D%(DEBUG)s',
                                        ]
        for define, value in self.py_module_defines:
            py_cflags_list.append( '/D%s=%s' % (define, value) )

        py_cflags_list.extend( self._getDefines( '/D%s' ) )

        self._addVar( 'CCCFLAGS', ' '.join( py_cflags_list ) )

        ldlibs = [
                    r'odbc32.lib',
                    r'odbccp32.lib',
                    r'Rpcrt4.lib',
                    r'Mswsock.lib',
                    r'%(SVN_LIB)s\libsvn_client-1.lib',
                    r'%(SVN_LIB)s\libsvn_delta-1.lib',
                    r'%(SVN_LIB)s\libsvn_diff-1.lib',
                    r'%(SVN_LIB)s\libsvn_fs-1.lib',
                    r'%(SVN_LIB)s\libsvn_fs_fs-1.lib',
                    r'%(SVN_LIB)s\libsvn_ra-1.lib',
                    r'%(SVN_LIB)s\libsvn_ra_local-1.lib',
                    r'%(SVN_LIB)s\libsvn_ra_svn-1.lib',
                    r'%(SVN_LIB)s\libsvn_repos-1.lib',
                    r'%(SVN_LIB)s\libsvn_subr-1.lib',
                    r'%(SVN_LIB)s\libsvn_wc-1.lib',
                    r'%(SVN_LIB)s\apr-iconv\libapriconv-1.lib',
                    r'%(SVN_LIB)s\apr-util\libaprutil-1.lib',
                    r'%(SVN_LIB)s\apr-util\xml.lib',
                    r'%(SVN_LIB)s\apr\libapr-1.lib',
                    ]
        if os.path.exists( self.expand( r'%(SVN_LIB)s\serf\serf-1.lib' ) ):
            ldlibs.append( r'%(SVN_LIB)s\serf\serf-1.lib' )
        elif os.path.exists( self.expand( r'%(SVN_LIB)s\libsvn_ra_neon-1.lib' ) ):
            ldlibs.append( r'%(SVN_LIB)s\libsvn_ra_neon-1.lib' )
        else:
            raise SetupError( 'Cannot find serf or neon' )

        ldlibs.append( r'ws2_32.lib' )

        self._addVar( 'LDLIBS', ' '.join( ldlibs ) )

    def ruleAllTestCase( self, target_name, all_test_cases ):
        self.makePrint( '%s: %s' %
                (target_name
                , ' '.join( ['test-%s.win32.new.log.clean' % (tc.test_name,) for tc in all_test_cases] )) )

    def ruleTestCase( self, test_case ):
        v = {'TN': test_case.test_name
            ,'KGV':  'py%d-svn%d.%d' %
                        (sys.version_info[0]
                        ,self.getSvnVersion()[0], self.getSvnVersion()[1])
            ,'SVN_VERSION': '%d.%d.%d' % self.getSvnVersion()
            }

        rules = []
        rules.append( '' )
        rules.append( '' )

        rules.append( 'test-%(TN)s.win32.new.log: test-%(TN)s.cmd test-%(TN)s.win32.known_good-%(KGV)s.log' % v )
        rules.append( '\t' '-subst b: /d >nul 2>&1' % v )
        rules.append( '\t' 'if exist testroot-%(TN)s rmdir /s /q testroot-%(TN)s' % v )
        rules.append( '\t' 'set PATH=%%(SVN_BIN)s;$(PATH)' % v )
        rules.append( '\t' 'set PYTHON=%%(PYTHON)s' % v )
        rules.append( '\t' 'test-%(TN)s.cmd >test-%(TN)s.win32.new.log 2>&1' % v )
        rules.append( '' )
        rules.append( 'test-%(TN)s.win32.new.log.clean: test-%(TN)s.win32.new.log' % v )
        rules.append( '\t' '%%(PYTHON)s benchmark_diff.py %(SVN_VERSION)s test-%(TN)s.win32.known_good-%(KGV)s.log test-%(TN)s.win32.new.log' % v )
        rules.append( '' )
        rules.append( 'clean-%(TN)s:' % v )
        rules.append( '\t' '-subst b: /d >nul 2>&1' % v )
        rules.append( '\t' 'if exist test-%(TN)s.win32.new.log del test-%(TN)s.win32.new.log' % v )
        rules.append( '\t' 'if exist testroot-%(TN)s rmdir /s /q testroot-%(TN)s' % v )
        rules.append( '' )
        rules.append( 'diff-%(TN)s: test-%(TN)s.win32.new.log' % v )
        rules.append( '\t' 'wb-diff.cmd test-%(TN)s.win32.known_good-%(KGV)s.log.clean test-%(TN)s.win32.new.log.clean' % v )
        rules.append( '' )
        rules.append( 'new-%(TN)s: test-%(TN)s.win32.new.log' % v )
        rules.append( '\t' 'copy test-%(TN)s.win32.new.log test-%(TN)s.win32.known_good-%(KGV)s.log' % v )

        self.makePrint( self.expand( '\n'.join( rules ) ) )


class CompilerGCC(Compiler):
    def __init__( self, setup ):
        Compiler.__init__( self, setup )

        self._addVar( 'CCC',            'g++' )
        self._addVar( 'CC',             'gcc' )

    def getPythonExtensionFileExt( self ):
        return '.so'

    def getProgramExt( self ):
        return ''

    def getObjectExt( self ):
        return '.o'

    def getTouchCommand( self ):
        return 'touch'

    def generateMakefileHeader( self ):
        self.makePrint( '#' )
        self.makePrint( '#        Pysvn Makefile generated by setup.py' )
        self.makePrint( '#' )
        self.makePrint( '' )

    def ruleLinkProgram( self, target ):
        target_filename = target.getTargetFilename()

        all_objects = [source.getTargetFilename() for source in target.all_sources]

        rules = []

        rules.append( '%s : %s' % (target_filename, ' '.join( all_objects )) )
        rules.append( '\t@echo Link %s' % (target_filename,) )
        rules.append( '\t@%%(LDEXE)s -o %s %%(CCCFLAGS)s %s' % (target_filename, ' '.join( all_objects )) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleLinkShared( self, target ):
        target_filename = target.getTargetFilename()

        all_objects = [source.getTargetFilename() for source in target.all_sources]

        rules = []

        rules.append( '%s : %s' % (target_filename, ' '.join( all_objects )) )
        rules.append( '\t@echo Link %s' % (target_filename,) )
        rules.append( '\t@%%(LDSHARED)s -o %s %%(CCCFLAGS)s %s %%(LDLIBS)s' % (target_filename, ' '.join( all_objects )) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleCxx( self, target ):
        obj_filename = target.getTargetFilename()

        rules = []

        rules.append( '%s: %s %s' % (obj_filename, target.src_filename, ' '.join( target.all_dependencies )) )
        rules.append( '\t@echo Compile: %s into %s' % (target.src_filename, obj_filename) )
        rules.append( '\t@%%(CCC)s -c %%(CCCFLAGS)s -o%s  %s' % (obj_filename, target.src_filename) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleC( self, target ):
        obj_filename = target.getTargetFilename()

        rules = []

        rules.append( '%s: %s %s' % (obj_filename, target.src_filename, ' '.join( target.all_dependencies )) )
        rules.append( '\t@echo Compile: %s into %s' % (target.src_filename, target) )
        rules.append( '\t@%%(CC)s -c %%(CCCFLAGS)s -o%s  %s' % (obj_filename, target.src_filename) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleClean( self, filename ):
        rules = []
        rules.append( 'clean::' )
        rules.append( '\trm -f %s' % (filename,) )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

    def ruleAllTestCase( self, target_name, all_test_cases ):
        self.makePrint( '%s: %s' %
                (target_name
                , ' '.join( ['test-%s.unix.new.log.clean' % (tc.test_name,) for tc in all_test_cases] )) )

    def ruleTestCase( self, test_case ):
        v = {'TN': test_case.test_name
            ,'KGV':  'py%d-svn%d.%d' %
                                        (sys.version_info[0]
                                        ,self.getSvnVersion()[0], self.getSvnVersion()[1])
            ,'SVN_VERSION': '%d.%d.%d' % self.getSvnVersion()
            }

        rules = []
        rules.append( '' )
        rules.append( '' )
        rules.append( 'test-%(TN)s.unix.new.log: test-%(TN)s.sh test-%(TN)s.unix.known_good-%(KGV)s.log' % v )
        rules.append( '\t' '-rm -rf testroot-%(TN)s' % v )
        rules.append( '\t' 'LD_LIBRARY_PATH=%%(SVN_LIB)s:%%(APR_LIB)s PATH=%%(SVN_BIN)s:$(PATH) PYTHON=%%(PYTHON)s ./test-%(TN)s.sh >test-%(TN)s.unix.new.log 2>&1' % v )
        rules.append( '' )
        rules.append( 'test-%(TN)s.unix.new.log.clean: test-%(TN)s.unix.new.log' % v )
        rules.append( '\t' '%%(PYTHON)s benchmark_diff.py %(SVN_VERSION)s test-%(TN)s.unix.known_good-%(KGV)s.log test-%(TN)s.unix.new.log' % v )
        rules.append( '' )
        rules.append( 'clean-%(TN)s:' % v )
        rules.append( '\t' '-rm -f test-%(TN)s.unix.new.log' % v )
        rules.append( '\t' '-rm -f test-%(TN)s.unix.new.log.clean' % v )
        rules.append( '\t' '-rm -rf testroot-%(TN)s' % v )
        rules.append( '' )
        rules.append( 'diff-%(TN)s: test-%(TN)s.unix.new.log.clean' % v )
        rules.append( '\t' 'wb-diff test-%(TN)s.unix.known_good-%(KGV)s.log.clean test-%(TN)s.unix.new.log.clean' % v )
        rules.append( '' )
        rules.append( 'new-%(TN)s: test-%(TN)s.unix.new.log' % v )
        rules.append( '\t' 'cp test-%(TN)s.unix.new.log test-%(TN)s.unix.known_good-%(KGV)s.log' % v )

        self.makePrint( self.expand( '\n'.join( rules ) ) )

class MacOsxCompilerGCC(CompilerGCC):
    def __init__( self, setup ):
        CompilerGCC.__init__( self, setup )

        if self.options.hasOption( '--arch' ):
            arch_options = ' '.join( ['-arch %s' % (arch,) for arch in self.options.getOption( '--arch' )] )
        else:
            arch_options = ''

        self._addVar( 'CCC',            'g++ %s' % (arch_options,) )
        self._addVar( 'CC',             'gcc %s' % (arch_options,) )

        self._find_paths_pycxx_dir = [
                        '../Import/pycxx-%d.%d.%d' % pycxx_version,
                        distutils.sysconfig.get_python_inc() # typical Linux
                        ]
        
        self._find_paths_pycxx_src = [
                        '%(PYCXX)s/Src',
                        '/usr/share/python%s/CXX' % (distutils.sysconfig.get_python_version(),) # typical Linux
                        ]
        self._find_paths_svn_inc = [
                        '/opt/local/include/subversion-1',      # Darwin - darwin ports
                        '/sw/include/subversion-1',             # Darwin - Fink
                        '/usr/include/subversion-1',            # typical Linux
                        '/usr/local/include/subversion-1',      # typical *BSD
                        '/usr/pkg/include/subversion-1',        # netbsd
                        ]
        self._find_paths_svn_bin = [
                        '/opt/local/bin',                        # Darwin - darwin ports
                        '/sw/bin',                                # Darwin - Fink
                        '/usr/bin',                                # typical Linux
                        '/usr/local/bin',                        # typical *BSD
                        '/usr/pkg/bin',                                # netbsd
                        ]
        self._find_paths_svn_lib = [
                        '/opt/local/lib',                       # Darwin - darwin ports
                        '/sw/lib',                              # Darwin - Fink
                        '/usr/lib64',                           # typical 64bit Linux
                        '/usr/lib',                             # typical Linux
                        '/usr/local/lib64',                     # typical 64bit Linux
                        '/usr/local/lib',                       # typical *BSD
                        '/usr/pkg/lib',                         # netbsd
                        ]
        self._find_paths_apr_inc = [
                        '/opt/local/include/apr-1',             # Darwin - darwin ports
                        '/sw/include/apr-1',                    # Darwin - fink
                        '/usr/include/apr-1',                   # typical Linux
                        '/usr/local/apr/include/apr-1',         # Mac OS X www.metissian.com
                        '/usr/pkg/include/apr-1',               # netbsd
                        '/usr/include/apache2',                 # alternate Linux
                        '/usr/include/httpd',                   # alternate Linux
                        '/usr/local/include/apr-1',             # typical FreeBSD
                        '/usr/local/include/apr0',              # typical *BSD
                        '/usr/local/include/apache2',           # alternate *BSD
                        ]
        self._find_paths_apr_util_inc = self._find_paths_apr_inc
        self._find_paths_apr_lib = [
                        '/opt/local/lib',                       # Darwin - darwin ports
                        '/sw/lib',                              # Darwin - fink
                        '/usr/lib64',                           # typical 64bit Linux
                        '/usr/lib',                             # typical Linux
                        '/usr/local/lib64',                     # typical 64bit Linux
                        '/usr/local/lib',                       # typical *BSD
                        '/usr/local/apr/lib',                   # Mac OS X www.metissian.com
                        '/usr/pkg/lib',                         # netbsd
                        ]

        self._completeInit()

    def get_lib_name_for_platform( self, libname ):
        return '%s.dylib' % (libname,)

    def setupUtilities( self ):
        self._addVar( 'CCCFLAGS',
                                        '-g  '
                                        '-no-long-double '
                                        '-Wall -fPIC -fexceptions -frtti '
                                        '-I. -I%(APR_INC)s -I%(SVN_INC)s '
                                        '-D%(DEBUG)s' )
        self._addVar( 'LDEXE',          '%(CCC)s -g' )

    def setupPySvn( self ):
        self._pysvnModuleSetup()
        self._addVar( 'PYSVN_MODULE_BASENAME', self.pysvn_module_name )

        self._addVar( 'PYTHON_VERSION',     '%d.%d' % (sys.version_info[0], sys.version_info[1]) )
        self._addVar( 'PYTHON_DIR',         sys.exec_prefix )
        self._addVar( 'PYTHON_FRAMEWORK',   os.path.join( sys.exec_prefix, 'Python' ) )
        self._addVar( 'PYTHON_INC',         distutils.sysconfig.get_python_inc() )

        py_cflags_list = [
                    '-g',
                    '-no-long-double',
                    '-Wall -fPIC -fexceptions -frtti',
                    '-I. -I%(APR_INC)s -I%(SVN_INC)s',
                    '-DPYCXX_PYTHON_2TO3 -I%(PYCXX)s -I%(PYCXX_SRC)s -I%(PYTHON_INC)s',
                    '-D%(DEBUG)s',
                    ]
        for define, value in self.py_module_defines:
            py_cflags_list.append( '-D%s=%s' % (define, value) )

        py_cflags_list.extend( self._getDefines( '-D%s' ) )

        py_ld_libs = [
                '-L%(SVN_LIB)s',
                '-L/usr/lib',
                '-lsvn_client-1',
                '-lsvn_repos-1',
                '-lsvn_wc-1',
                '-lsvn_fs-1',
                '-lsvn_subr-1',
                '-lsvn_diff-1',
                '-lapr-1',
                '-lssl',
                ]

        self._addVar( 'CCCFLAGS', ' '.join( py_cflags_list ) )
        self._addVar( 'LDLIBS', ' '.join( py_ld_libs ) )
        self._addVar( 'LDSHARED',       '%(CCC)s -bundle -g '
                                        '-framework System '
                                        '%(PYTHON_FRAMEWORK)s '
                                        '-framework CoreFoundation '
                                        '-framework Kerberos '
                                        '-framework Security '
                                        '%(LDLIBS)s' )

class UnixCompilerGCC(CompilerGCC):
    def __init__( self, setup ):
        CompilerGCC.__init__( self, setup )

        self._find_paths_pycxx_dir = [
                        '../Import/pycxx-%d.%d.%d' % pycxx_version,
                        distutils.sysconfig.get_python_inc() # typical Linux
                        ]
        self._find_paths_pycxx_src = [
                        '%(PYCXX)s/Src',
                        '/usr/share/python%s/CXX' % distutils.sysconfig.get_python_version() # typical Linux
                        ]
        self._find_paths_svn_inc = [
                        '/usr/include/subversion-1',            # typical Linux
                        '/usr/local/include/subversion-1',      # typical *BSD
                        '/usr/pkg/include/subversion-1',        # netbsd
                        ]
        self._find_paths_svn_bin = [
                        '/usr/bin',                                # typical Linux
                        '/usr/local/bin',                        # typical *BSD
                        '/usr/pkg/bin',                                # netbsd
                        ]
        self._find_paths_svn_lib = [
                        '/usr/lib64',                           # typical 64bit Linux
                        '/usr/lib',                             # typical Linux
                        '/usr/local/lib64',                     # typical 64bit Linux
                        '/usr/local/lib',                       # typical *BSD
                        '/usr/pkg/lib',                         # netbsd
                        ]
        self._find_paths_apr_inc = [
                        '/usr/include/apr-1',                   # typical Linux
                        '/usr/local/apr/include/apr-1',         # Mac OS X www.metissian.com
                        '/usr/pkg/include/apr-1',               # netbsd
                        '/usr/include/apache2',                 # alternate Linux
                        '/usr/include/httpd',                   # alternate Linux
                        '/usr/local/include/apr-1',             # typical *BSD
                        '/usr/local/include/apache2',           # alternate *BSD
                        ]
        self._find_paths_apr_util_inc = self._find_paths_apr_inc
        self._find_paths_apr_lib = [
                        '/usr/lib64',                           # typical 64bit Linux
                        '/usr/lib',                             # typical Linux
                        '/usr/local/lib64',                     # typical 64bit Linux
                        '/usr/local/lib',                       # typical *BSD
                        '/usr/local/apr/lib',                   # Mac OS X www.metissian.com
                        '/usr/pkg/lib',                         # netbsd
                        ]

        self._completeInit()

    def get_lib_name_for_platform( self, libname ):
        if self.setup.platform == 'cygwin':
            return '%s.dll.a' % libname
        else:
            return '%s.so' % libname

    def setupUtilities( self ):
        self._addVar( 'CCCFLAGS',
                                        '-g  '
                                        '-Wall -fPIC -fexceptions -frtti '
                                        '-I. -I%(APR_INC)s -I%(SVN_INC)s '
                                        '-D%(DEBUG)s' )
        self._addVar( 'LDEXE',          '%(CCC)s -g' )

    def setupPySvn( self ):
        self._pysvnModuleSetup()
        self._addVar( 'PYSVN_MODULE_BASENAME', self.pysvn_module_name )

        self._addVar( 'PYTHON_VERSION', '%d.%d' % (sys.version_info[0], sys.version_info[1]) )
        self._addVar( 'PYTHON_INC',     distutils.sysconfig.get_python_inc( False ) )
        self._addVar( 'PYTHON_ARCH_SPECIFIC_INC',     distutils.sysconfig.get_python_inc( True ) )

        py_cflags_list = [
                    '-Wall -fPIC -fexceptions -frtti',
                    '-I. -I%(APR_INC)s -I%(SVN_INC)s',
                    '-DPYCXX_PYTHON_2TO3 -I%(PYCXX)s -I%(PYCXX_SRC)s -I%(PYTHON_INC)s',
                    '-I%(PYTHON_ARCH_SPECIFIC_INC)s',
                    '-D%(DEBUG)s',
                    ]
        for define, value in self.py_module_defines:
            py_cflags_list.append( '-D%s=%s' % (define, value) )

        py_cflags_list.extend( self._getDefines( '-D%s' ) )

        if self.options.hasOption( '--enable-debug' ):
            print( 'Info: Debug enabled' )
            py_cflags_list.append( '-g' )

        self._addVar( 'CCCFLAGS',   ' '.join( py_cflags_list ) )
        self._addVar( 'LDLIBS',     ' '.join( self._getLdLibs() ) )
        self._addVar( 'LDSHARED',   '%(CCC)s -shared -g' )

#--------------------------------------------------------------------------------
class LinuxCompilerGCC(UnixCompilerGCC):
    def __init__( self, setup ):
        UnixCompilerGCC.__init__( self, setup )

    def _getLdLibs( self ):
        py_ld_libs = [
                '-L%(SVN_LIB)s',
                ]
        if not self.setup.options.hasOption( '--norpath' ):
            py_ld_libs.extend( [
                    '-Wl,--rpath',
                    '-Wl,%(SVN_LIB)s'
                    ] )
        py_ld_libs.extend( [
                '-lsvn_client-1',
                '-lsvn_repos-1',
                '-lsvn_wc-1',
                '-lsvn_fs-1',
                '-lsvn_subr-1',
                '-lsvn_diff-1',
                '-lapr-1',
                ] )
        return py_ld_libs

class FreeBsdCompilerGCC(UnixCompilerGCC):
    def __init__( self, setup ):
        UnixCompilerGCC.__init__( self, setup )

    def _getLdLibs( self ):
        py_ld_libs = [
                '-L%(SVN_LIB)s',
                '-Wl,--rpath',
                '-Wl,/usr/lib:/usr/local/lib',
                '-lsvn_client-1',
                '-lsvn_diff-1',
                '-lsvn_repos-1',
                ]

        if os.path.exists( '/usr/lib/libkrb5.so' ):
            py_ld_libs.append( '-lkrb5' )

        py_ld_libs.extend( [
                '-lcom_err',
                '-lexpat',
                '-lneon',
                ] )
        return py_ld_libs

class CygwinCompilerGCC(UnixCompilerGCC):
    def __init__( self, setup ):
        UnixCompilerGCC.__init__( self, setup )

    def _getLdLibs( self ):
        py_ld_libs = [
                '-L%(svn_lib_dir)s',
                '-L/usr/lib/python%d.%d/config -lpython%d.%d.dll' %
                    (sys.version_info[0], sys.version_info[1], sys.version_info[0], sys.version_info[1]),
                '-lsvn_client-1',
                '-lsvn_repos-1',
                '-lsvn_subr-1',
                '-lsvn_delta-1',
                '-lsvn_fs_fs-1',
                '-lsvn_fs-1',
                '-lsvn_ra_svn-1',
                '-lsvn_repos-1',
                '-lsvn_ra_local-1',
                '-lsvn_diff-1',
                '-lsvn_ra-1',
                '-lsvn_wc-1',
                '-lapr-1',
                '-laprutil-1',
                '-liconv',
                '-lexpat',
                '-lpthread',
                '-lz',
                ]
        return py_ld_libs

class AixCompilerGCC(UnixCompilerGCC):
    def __init__( self, setup ):
        UnixCompilerGCC.__init__( self, setup )

    def _getLdLibs( self ):
        for path in sys.path:
            python_exp = os.path.join( path, 'config', 'python.exp' )
            if os.path.exists( python_exp ):
                break
        else:
            python_exp = os.path.abspath( os.path.join( sys.executable, os.path.pardir, os.path.pardir, 
                                              'lib', 'python%d.%d' % (sys.version_info[0], sys.version_info[1]), 'config', 'python.exp'))
            if not os.path.exists(python_exp):
                python_exp = 'python.exp'

        py_ld_libs = [
                '-L%(svn_lib_dir)s',
                '-lsvn_client-1',
                '-lsvn_repos-1',
                '-lsvn_subr-1',
                '-lsvn_delta-1',
                '-lsvn_fs_fs-1',
                '-lsvn_fs-1',
                '-lsvn_ra_svn-1',
                '-lsvn_repos-1',
                '-lsvn_ra_local-1',
                '-lsvn_diff-1',
                '-lsvn_ra-1',
                '-lsvn_wc-1',
                '-lsvn_fs_util-1',
                '-lsvn_ra_neon-1',
                '-lapr-1',
                '-lneon',
                '-laprutil-1',
                '-liconv',
                '-lexpat',
                '-lintl',
                '-lpthread',
                '-lz',
                '-Wl,-bI:%s' % (python_exp,),
                ]
        return py_ld_libs

class SunOsCompilerGCC(UnixCompilerGCC):
    def __init__( self, setup ):
        UnixCompilerGCC.__init__( self, setup )

    def _getLdLibs( self ):
        py_ld_libs = [
                '-L%(svn_lib_dir)s',
                '-Wl,--rpath -Wl,%(svn_lib_dir)s',
                '-lsvn_client-1',
                '-lsvn_diff-1',
                '-lsvn_repos-1',
                '-lresolv',
                '-lexpat',
                '-lneon',
                ]
        return py_ld_libs

#--------------------------------------------------------------------------------
class Target:
    def __init__( self, compiler, all_sources ):
        self.compiler = compiler
        self.__generated = False
        self.dependent = None


        self.all_sources = all_sources
        for source in self.all_sources:
            source.setDependent( self )

    def getTargetFilename( self ):
        raise NotImplementedError( '%s.getTargetFilename' % self.__class__.__name__ )

    def generateMakefile( self ):
        if self.__generated:
            return

        self.__generated = True
        return self._generateMakefile()

    def _generateMakefile( self ):
        raise NotImplementedError( '_generateMakefile' )

    def ruleClean( self, ext=None ):
        if ext is None:
            target_filename = self.getTargetFilename()
        else:
            target_filename = self.getTargetFilename( ext )

        self.compiler.ruleClean( target_filename )

    def setDependent( self, dependent ):
        debug( '%r.setDependent( %r )' % (self, dependent,) )
        self.dependent = dependent


class Program(Target):
    def __init__( self, compiler, output, all_sources ):
        self.output = output

        Target.__init__( self, compiler, all_sources )
        debug( 'Program:0x%8.8x.__init__( %r, ... )' % (id(self), output,) )

    def __repr__( self ):
        return '<Program:0x%8.8x %s>' % (id(self), self.output)

    def getTargetFilename( self, ext=None ):
        if ext is None:
            ext = self.compiler.getProgramExt()

        return self.compiler.platformFilename( self.compiler.expand( '%s%s' % (self.output, ext) ) )

    def _generateMakefile( self ):
        debug( 'Program:0x%8.8x.generateMakefile() for %r dependent %r' % (id(self), self.output, self.dependent) )

        self.compiler.ruleLinkProgram( self )

        self.compiler.ruleClean( self.getTargetFilename() )

        for source in self.all_sources:
            source.generateMakefile()


class PythonExtension(Target):
    def __init__( self, compiler, output, all_sources ):
        self.output = output

        Target.__init__( self, compiler, all_sources )
        debug( 'PythonExtension:0x%8.8x.__init__( %r, ... )' % (id(self), output,) )

        for source in self.all_sources:
            source.setDependent( self )

    def __repr__( self ):
        return '<PythonExtension:0x%8.8x %s>' % (id(self), self.output)

    def getTargetFilename( self, ext=None ):
        if ext is None:
            ext = self.compiler.getPythonExtensionFileExt()
        return self.compiler.platformFilename( self.compiler.expand( '%s%s' % (self.output, ext) ) )

    def _generateMakefile( self ):
        debug( 'PythonExtension:0x%8.8x.generateMakefile() for %r' % (id(self), self.output,) )

        self.compiler.ruleLinkShared( self )
        self.compiler.ruleClean( self.getTargetFilename( '.*' ) )

        for source in self.all_sources:
            source.generateMakefile()

class Source(Target):
    def __init__( self, compiler, src_filename, all_dependencies=None ):
        self.src_filename = compiler.platformFilename( compiler.expand( src_filename ) )

        Target.__init__( self, compiler, [] )

        debug( 'Source:0x%8.8x.__init__( %r, %r )' % (id(self), src_filename, all_dependencies) )

        self.all_dependencies = all_dependencies
        if self.all_dependencies is None:
            self.all_dependencies = []

    def __repr__( self ):
        return '<Source:0x%8.8x %s>' % (id(self), self.src_filename)

    def makePrint( self, line ):
        self.compiler.makePrint( line )

    def getTargetFilename( self ):
        #if not os.path.exists( self.src_filename ):
        #    raise SetupError( 'Cannot find source %s' % (self.src_filename,) )

        obj_ext = self.compiler.getObjectExt()

        basename = os.path.basename( self.src_filename )
        if basename.endswith( '.cpp' ):
            return self.compiler.platformFilename( self.compiler.expand( '%s%s' % (basename[:-len('.cpp')], obj_ext) ) )

        if basename.endswith( '.cxx' ):
            return self.compiler.platformFilename( self.compiler.expand( '%s%s' % (basename[:-len('.cxx')], obj_ext) ) )

        if basename.endswith( '.c' ):
            return self.compiler.platformFilename( self.compiler.expand( '%s%s' % (basename[:-len('.c')], obj_ext) ) )

        raise SetupError( 'unknown source %r' % (self.src_filename,) )

    def _generateMakefile( self ):
        debug( 'Source:0x%8.8x.generateMakefile() for %r' % (id(self), self.src_filename,) )

        self.compiler.ruleCxx( self )
        self.compiler.ruleClean( self.getTargetFilename() )

class PysvnSvnErrorsPy(Source):
    def __init__( self, compiler ):
        Source.__init__( self, compiler, [] )

        debug( 'PysvnSvnErrorsPy:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnSvnErrorsPy:0x%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( '' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s %s' %
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( '../Docs/generate_cpp_docs_from_html_docs.py' )
                    ,self.compiler.platformFilename( '../Docs/pysvn_prog_ref.html' )) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )
        rules.append( '\t%%(PYTHON)s -u %s %%(SVN_INC)s %s %s pysvn_docs.hpp pysvn_docs.cpp' % 
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( '../Docs/generate_cpp_docs_from_html_docs.py' )
                    ,self.compiler.platformFilename( '../Docs/pysvn_prog_ref.html' )) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class PysvnDocsSource(Source):
    def __init__( self, compiler ):
        Source.__init__( self, compiler, 'pysvn_docs.cpp', [] )

        debug( 'PysvnDocsSource:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnDocsSource:0x%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( 'pysvn_docs.cpp' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s %s' %
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( '../Docs/generate_cpp_docs_from_html_docs.py' )
                    ,self.compiler.platformFilename( '../Docs/pysvn_prog_ref.html' )) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )
        rules.append( '\t%%(PYTHON)s -u %s %%(SVN_INC)s %s pysvn_docs.hpp pysvn_docs.cpp' % 
                    (self.compiler.platformFilename( '../Docs/generate_cpp_docs_from_html_docs.py' )
                    ,self.compiler.platformFilename( '../Docs/pysvn_prog_ref.html' )) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class GenerateSvnErrorCodesHeader(Source):
    def __init__( self, compiler ):
        Source.__init__( self, compiler, 'generate_svn_error_codes.hpp', [] )

        debug( 'GenerateSvnErrorCodesHeader:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnDocsSource:GenerateSvnErrorCodesHeader%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( 'generate_svn_error_codes.hpp' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s' %
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( 'generate_svn_error_codes/create_svn_error_codes_hpp.py' )) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )
        rules.append( '\t%%(PYTHON)s -u %s %%(SVN_INC)s' % 
                    (self.compiler.platformFilename( 'generate_svn_error_codes/create_svn_error_codes_hpp.py' ),) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class PysvnDocsHeader(Source):
    def __init__( self, compiler, all_dependencies ):
        Source.__init__( self, compiler, 'pysvn_docs.hpp', [] )

        self.all_dependencies = all_dependencies

        debug( 'PysvnDocsHeader:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnDocsHeader:0x%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( 'pysvn_docs.hpp' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s' %
                    (self.getTargetFilename()
                    ,' '.join( self.all_dependencies)) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )
        rules.append( '\t%s %s' %
                    (self.compiler.getTouchCommand()
                    ,self.getTargetFilename(),) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class PysvnVersionHeader(Source):
    def __init__( self, compiler ):
        Source.__init__( self, compiler, 'pysvn_version.hpp', [] )

        debug( 'PysvnVersionHeader:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnVersionHeader:0x%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( 'pysvn_version.hpp' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s %s %s' %
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( '../Builder/brand_version.py' )
                    ,self.compiler.platformFilename( '../Builder/version.info' )
                    ,self.compiler.platformFilename( 'pysvn_version.hpp.template' )) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )

        rules.append( '\t%%(PYTHON)s -u %s %s %s' % 
                    (self.compiler.platformFilename( '../Builder/brand_version.py' )
                    ,self.compiler.platformFilename( '../Builder/version.info' )
                    ,self.compiler.platformFilename( 'pysvn_version.hpp.template' )) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class PysvnModuleInit(Source):
    def __init__( self, compiler ):
        Source.__init__( self, compiler, 'pysvn/__init__.py', [] )

        debug( 'PysvnModuleInit:0x%8.8x.__init__()' % (id(self),) )

    def __repr__( self ):
        return '<PysvnModuleInit:0x%8.8x>' % (id(self),)

    def getTargetFilename( self ):
        return self.compiler.platformFilename( 'pysvn/__init__.py' )

    def generateMakefile( self ):
        rules = ['']

        rules.append( '%s : %s %s %s' %
                    (self.getTargetFilename()
                    ,self.compiler.platformFilename( 'pysvn/__init__.py.template' )
                    ,self.compiler.platformFilename( 'create__init__.py' )
                    ,self.compiler.platformFilename( 'generate_svn_error_codes/generate_svn_error_codes%s' % 
                                                        (self.compiler.getProgramExt(),) )) )
        rules.append( '\t@ echo Info: Make %s' % self.getTargetFilename() )

        rules.append( '\t%%(PYTHON)s -u %s %s %s %s %%(PYSVN_MODULE_BASENAME)s%s' % 
                    (self.compiler.platformFilename( 'create__init__.py' )
                    ,self.compiler.platformFilename( 'pysvn/__init__.py.template' )
                    ,self.getTargetFilename()
                    ,self.compiler.platformFilename( 'generate_svn_error_codes/generate_svn_error_codes%s' % 
                                                        (self.compiler.getProgramExt(),) )
                    ,self.compiler.getPythonExtensionFileExt()) )

        self.makePrint( self.compiler.expand( '\n'.join( rules ) ) )
        self.ruleClean()

class TestCase(Target):
    def __init__( self, compiler, test_name ):
        self.test_name = test_name

        Target.__init__( self, compiler, [] )

        debug( 'TestCase:0x%8.8x.__init__( %r )' % (id(self), test_name) )

    def __repr__( self ):
        return '<TestCase:0x%8.8x %s>' % (id(self), self.test_name)

    def _generateMakefile( self ):
        debug( 'TestCase:0x%8.8x.generateMakefile() for %r' % (id(self), self.test_name) )

        self.compiler.ruleTestCase( self )
