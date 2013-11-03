
import os
import commands
import string

EnsureSConsVersion(0, 98, 1)

BUILD_PATH = '#/build/'
#CORE_PACKAGE = 'libairdcpp'
PACKAGE = 'airdcnano'

BUILD_FLAGS = {
	'common'  : ['-I#', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-D__cdecl=""', '-std=c++11', '-Wfatal-errors', '-fexceptions', '-Wno-reorder', '-Wno-overloaded-virtual'],
	'debug'   : ['-g', '-ggdb', '-D_DEBUG'], 
	'release' : ['-O3', '-fomit-frame-pointer', '-DNDEBUG'],
	'beta' : ['-g', '-DNDEBUG']
}

def check_pkg_config(context):
	context.Message('Checking for pkg-config... ')
	ret = context.TryAction('pkg-config --version')[0]
	context.Result(ret)
	return ret

def check_pkg(context, name):
	context.Message('Checking for %s... ' % name)
	ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
	context.Result(ret)
	return ret

def check_cxx_version(context, name, major, minor):
	context.Message('Checking for %s >= %d.%d...' % (name, major, minor))
	ret = commands.getoutput('%s -dumpversion' % name)

	retval = 0
	try:
		if ((string.atoi(ret[0]) == major and string.atoi(ret[2]) >= minor)
		or (string.atoi(ret[0]) > major)):
			retval = 1
	except ValueError:
		print "No C++ compiler found!"

	context.Result(retval)
	return retval
	
	
# ----------------------------------------------------------------------
# Command-line options
# ----------------------------------------------------------------------

# Parameters are only sticky from scons -> scons install, otherwise they're cleared.
if 'install' in COMMAND_LINE_TARGETS:
	vars = Variables('build/sconf/scache.conf')
else:
	vars = Variables()

vars.AddVariables(
	BoolVariable('debug', 'Compile the program with debug information', 0),
	BoolVariable('release', 'Compile the program with optimizations', 0),
	BoolVariable('profile', 'Compile the program with profiling information', 0),
	PathVariable('PREFIX', 'Compile the program with PREFIX as the root for installation', '/usr/local', PathVariable.PathIsDir),
	('FAKE_ROOT', 'Make scons install the program under a fake root', '')
)


# ----------------------------------------------------------------------
# Initialization
# ----------------------------------------------------------------------

env = Environment(ENV = os.environ, variables = vars, package = PACKAGE)

if env.get('debug'):
	env['mode'] = 'debug'
elif env.get('release'):
	env['mode'] = 'release'
else:
	env['mode'] = 'beta'


env['build_path'] = BUILD_PATH + env['mode'] + '/'

if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
elif os.name == 'mac':
	print 'CXX env variable is not set, attempting to use clang'
	env['CXX'] = 'clang'
else:
	print 'CXX env variable is not set, attempting to use g++'
	env['CXX'] = 'g++'

if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']

if os.environ.has_key('CXXFLAGS'):
	env['CPPFLAGS'] = env['CXXFLAGS'] = os.environ['CXXFLAGS'].split()

if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] = os.environ['LDFLAGS'].split()

if os.environ.has_key('CFLAGS'):
	env['CFLAGS'] = os.environ['CFLAGS'].split()

env['CPPDEFINES'] = [] # Initialize as a list so Append doesn't concat strings

env.SConsignFile('build/sconf/.sconsign')
vars.Save('build/sconf/scache.conf', env)


conf = env.Configure(
	custom_tests =
	{
		'CheckPKGConfig' : check_pkg_config,
		'CheckPKG' : check_pkg,
		'CheckCXXVersion' : check_cxx_version
	},
	conf_dir = 'build/sconf',
	log_file = 'build/sconf/config.log')

	
	
# ----------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------

if not 'install' in COMMAND_LINE_TARGETS:
	Execute('sh generate-version.sh') 
	if env['CXX'] == 'clang':
		if not conf.CheckCXXVersion(env['CXX'], 3, 3):
			 print 'Compiler version check failed. Supported compilers: clang 3.3 or later, g++ 4.7 or later'
			 Exit(1)

	elif not conf.CheckCXXVersion(env['CXX'], 4, 7):
		 print 'Compiler version check failed. Supported compilers: clang 3.3 or later, g++ 4.7 or later'
		 Exit(1)

	# Add support for compiler caches to speed-up compilation.
	if conf.TryAction(Action('distcc'))[0]:
		env.Prepend(CXX = 'distcc ')
		print 'Enabling distcc...'

	if conf.TryAction(Action('ccache'))[0]:
		env.Prepend(CXX = 'ccache ')
		print 'Enabling ccache...'

	if not conf.CheckPKGConfig(): 
		 print 'pkg-config not found.' 
		 Exit(1) 

	if conf.CheckLibWithHeader('ncursesw', 'ncursesw/ncurses.h', 'c'):
		env.Prepend(CXXFLAGS = ['-DHAVE_NCURSESW_NCURSES_H'])
	elif conf.CheckLibWithHeader('ncursesw', 'ncurses/ncurses.h', 'c'): 
		env.Prepend(CXXFLAGS = ['-DHAVE_NCURSES_NCURSES_H'])
	elif not conf.CheckLibWithHeader('ncursesw', 'ncurses.h', 'c'): 
		print 'Did not find the ncursesw library'
		print 'Can\'t live without it, exiting'
		print 'Note: You might have the lib but not the headers (install dev-package)'
		Exit(1)


	if not conf.CheckLibWithHeader('pthread', 'pthread.h', 'c++'):
		print 'Did not find the pthread library, exiting!'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('z', 'zlib.h', 'c++'):
		print 'Did not find the z library (gzip/z compression)'
		print 'Can\'t live without it, exiting'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'c++'):
		print 'Did not find the bz2 library (bz2 compression)'
		print 'Can\'t live without it, exiting'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckPKG('glib-2.0'):
		print 'Did not find the glib-2.0 library'
		print 'Can\'t live without it, exiting'
		print 'Note: You might have the lib but not the development package (libglib2.0-dev)'
		Exit(1)

	if not conf.CheckLib('boost_signals') and not conf.CheckLib('boost_signals-mt'):
		print 'Did not find the boost_signals library, exiting!'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_thread') and not conf.CheckLib('boost_thread-mt'):
		print 'Did not find the boost_thread library, exiting!'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_regex') and not conf.CheckLib('boost_regex-mt'):
		print 'Did not find the boost_regex library, exiting!'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLib('boost_system') and not conf.CheckLib('boost_system-mt'):
		print 'Did not find the boost_system library, exiting!'
		print 'Note: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckCXXHeader('boost/version.hpp', '<>'):
		print '\tboost not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('leveldb', 'leveldb/db.h', 'cpp'):
		print '\tleveldb not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)
		
	if conf.CheckCXXHeader('leveldb/filter_policy.h'):
		conf.env.Append(CPPDEFINES = 'HAVE_LEVELDB_BLOOM')

	if not conf.CheckLibWithHeader('GeoIP', 'GeoIP.h', 'c'):
		print '\tGeoIP not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('tbb', 'tbb/tbb.h', 'cpp'):
		print '\tTBB not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if conf.CheckLibWithHeader('natpmp', 'natpmp.h', 'c'):
		conf.env.Append(CPPDEFINES = 'HAVE_NATPMP_H')

	if not conf.CheckLibWithHeader('miniupnpc', 'miniupnpc/miniupnpc.h', 'c'):
		print '\tminiupnpc not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)
	if not conf.CheckLibWithHeader('miniupnpc', 'miniupnpc/miniupnpc.h', 'c', 'upnpDiscover(0, "", 0, 0, 0, 0);'):
		conf.env.Append(CPPDEFINES = 'HAVE_OLD_MINIUPNPC')

	# This needs to be before ssl check on *BSD systems
	if not conf.CheckLib('crypto'):
		print '\tcrypto library not found'
		print '\tNote: This library may be a part of libssl on your system'
		Exit(1)

	if not conf.CheckLibWithHeader('ssl', 'openssl/ssl.h', 'c'):
		print '\tOpenSSL library (libssl) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)
		
	if not conf.CheckHeader('iconv.h'):
		Exit(1)
	elif conf.CheckLibWithHeader('iconv', 'iconv.h', 'c', 'iconv(0, (const char **)0, 0, (char**)0, 0);'):
		if os.name == 'mac' or os.sys.platform == 'darwin':
			conf.env.Append(CPPDEFINES = ('ICONV_CONST', ''))
		else:
			conf.env.Append(CPPDEFINES = ('ICONV_CONST', 'const'))

	if not conf.CheckLib('stdc++'):
		print '\tstdc++ library not found'
		Exit(1)

	if not conf.CheckLib('m'):
		print '\tmath library not found'
		Exit(1)

	if not conf.CheckFunc('backtrace'):
		conf.env.Prepend(CXXFLAGS = '-DUSE_STACKTRACE=0')
	else:
		conf.env.Prepend(CXXFLAGS = '-DUSE_STACKTRACE=1')
		
	if conf.CheckHeader(['sys/types.h', 'sys/socket.h', 'ifaddrs.h', 'net/if.h']):
		conf.env.Append(CPPDEFINES = 'HAVE_IFADDRS_H')

	env = conf.Finish()

# ----------------------------------------------------------------------
# Compile and link flags
# ----------------------------------------------------------------------

	env.MergeFlags(BUILD_FLAGS['common'])
	env.MergeFlags(BUILD_FLAGS[env['mode']])
	env.ParseConfig('pkg-config --libs gthread-2.0')

	#env.Append(LIBPATH = env['build_path'] + CORE_PACKAGE)
	#env.Prepend(LIBS = 'airdcpp')

	if os.sys.platform == 'linux2':
		env.Append(LINKFLAGS = '-Wl,--as-needed')

	if os.sys.platform == 'sunos5':
	#	conf.env.Append(CPPDEFINES = ('ICONV_CONST', 'const'))
		env.Append(LIBS = ['socket', 'nsl'])

	if env.get('profile'):
		env.Append(CXXFLAGS = '-pg')
		env.Append(LINKFLAGS= '-pg')

	if env.get('PREFIX'):
		data_dir = '\'\"%s/share\"\'' % env['PREFIX']
		env.Append(CPPDEFINES = ('_DATADIR', data_dir))


	#env.Prepend(CXXFLAGS = ['-I.', '-ansi', '-Wall'])
	#env.Append(CXXFLAGS = commands.getoutput('pkg-config sigc++-2.0 --cflags').split())
	env.Prepend(CXXFLAGS = commands.getoutput('pkg-config glib-2.0 --cflags').split())
	#env.Append(_LIBFLAGS = ' ' + commands.getoutput('pkg-config sigc++-2.0 --libs'))
	env.Append(_LIBFLAGS = ' ' + commands.getoutput('pkg-config glib-2.0 --libs'))

	#env.Append(LIBPATH = env['build_path'])

	#	env.MergeFlags(BUILD_FLAGS['common'])
	#	env.MergeFlags(BUILD_FLAGS[env['mode']])
	
# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------	

	build = env.Program('airdcnano', [
		SConscript('client/SConscript', exports='env', variant_dir= env['build_path'] + 'client', duplicate=0),
		SConscript('core/SConscript', exports='env', variant_dir= env['build_path'] + 'core', duplicate=0),
		SConscript('input/SConscript', exports='env', variant_dir= env['build_path'] + 'input', duplicate=0),
		SConscript('utils/SConscript', exports='env', variant_dir= env['build_path'] + 'utils', duplicate=0),
		SConscript('ui/SConscript', exports='env', variant_dir= env['build_path'] + 'ui', duplicate=0),
		SConscript('display/SConscript', exports='env', variant_dir= env['build_path'] + 'display', duplicate=0),
		SConscript('modules/SConscript', exports='env', variant_dir= env['build_path'] + 'commands', duplicate=0),
	])

	Default(build)
	
# ----------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------
	
else:

	#glade_files = env.Glob('glade/*.glade')
	#text_files = env.Glob('*.txt')
	prefix = env['FAKE_ROOT'] + env['PREFIX']
	#desktop_file = os.path.join('data', PACKAGE + '.desktop')
	#app_icon_filter = lambda icon: os.path.splitext(icon)[0] == PACKAGE
	#regular_icon_filter = lambda icon: os.path.splitext(icon)[0] != PACKAGE

	#env.RecursiveInstall('icons/hicolor', os.path.join(prefix, 'share', 'icons'), app_icon_filter)
	#env.RecursiveInstall('icons/hicolor', os.path.join(prefix, 'share', PACKAGE, 'icons'), regular_icon_filter)
	#env.RecursiveInstall(BUILD_LOCALE_PATH, os.path.join(prefix, 'share', 'locale'))

	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', PACKAGE, 'glade'), source = glade_files))
	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', 'doc', PACKAGE), source = text_files))
	#env.Alias('install', env.Install(dir = os.path.join(prefix, 'share', 'applications'), source = desktop_file))
	env.Alias('install', env.Install(dir = os.path.join(prefix, 'bin'), source = PACKAGE))

