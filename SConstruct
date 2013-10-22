
import os
import commands
import string

EnsureSConsVersion(0, 96)

def CheckPKGConfig(context): 
     context.Message('Checking for pkg-config... ') 
     ret = context.TryAction('pkg-config --version')[0] 
     context.Result(ret)
     return ret 
 
def CheckPKG(context, name): 
     context.Message('Checking for %s... ' % name) 
     ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0] 
     context.Result(ret) 
     return ret 

def CheckCXXVersion(context, name, major, minor):
    context.Message('Checking for %s >= %d.%d...' % (name, major, minor))

    ret = commands.getoutput('%s -dumpversion' % name)
    retval = 0
    if(ret.find('not found') == -1):
        if(string.atoi(ret[0]) == major and string.atoi(ret[2]) >= minor):
            retval = 1
        elif(string.atoi(ret[0]) > major):
            retval = 1
                
    context.Result(retval)
    return retval

env = Environment(ENV = os.environ)

Execute('sh generate-version.sh') 
#env.Alias('Run', env.Command('run.dummy', [], 'ls -l'))
#from subprocess import call
#call("sh git.sh")

#env.Command('sh git.sh')
#env.Append(LIBPATH = '/opt/local/lib/')

conf = Configure(env,
    custom_tests = {
        'CheckPKGConfig' : CheckPKGConfig, 
        'CheckPKG' : CheckPKG,
        'CheckCXXVersion' : CheckCXXVersion
    }) 

if os.environ.has_key('CXX'):
     env['CXX'] = os.environ['CXX'] 
else:
	print 'CXX env variable is not set, attempting to use g++' 
	env['CXX'] = 'g++'

if os.environ.has_key('CXXFLAGS'):
	env['CXXFLAGS'] = os.environ['CXXFLAGS'].split()
	
release = ARGUMENTS.get('release', 0)
debug = ARGUMENTS.get('debug', 0)

if int(release):
    env.Append(CXXFLAGS = ['-O2',  '-I#', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-D__cdecl=""', '-std=c++11', '-Wfatal-errors', '-fexceptions', '-Wno-reorder', '-Wno-overloaded-virtual'])
elif int(debug):
    env.Append(CXXFLAGS = ['-g', '-I#', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-D__cdecl=""', '-std=c++11', '-Wfatal-errors', '-fexceptions', '-Wno-reorder', '-Wno-overloaded-virtual'])
else:
    env.Append(CXXFLAGS = ['-g', '-I#', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-D__cdecl=""', '-std=c++11', '-Wfatal-errors', '-fexceptions', '-Wno-reorder', '-Wno-overloaded-virtual'])

	
	
# Dependencies
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
elif conf.CheckLibWithHeader('ncurses', 'ncurses/ncurses.h', 'c'): 
	env.Prepend(CXXFLAGS = ['-DHAVE_NCURSES_NCURSES_H'])
elif not conf.CheckLibWithHeader('ncurses', 'ncurses.h', 'c'): 
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

#if not conf.CheckPKG('sigc++-2.0'):
#    print 'Did not find the sigc++-2.0 library'
#    print 'Can\'t live without it, exiting'
#    Exit(1)

#if not conf.CheckLibWithHeader('boost_signals', 'boost/signals.hpp', 'c++'):
#    print 'Did not find the boost_signals library, exiting!'
#    print 'Note: You might have the lib but not the headers'
#    Exit(1)

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

# This needs to be before ssl check on *BSD systems
if not conf.CheckLib('crypto'):
	print '\tcrypto library not found'
	print '\tNote: This library may be a part of libssl on your system'
	Exit(1)

if not conf.CheckLibWithHeader('ssl', 'openssl/ssl.h', 'c'):
	print '\tOpenSSL library (libssl) not found'
	print '\tNote: You might have the lib but not the headers'
	Exit(1)

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
env.Prepend(CXXFLAGS = ['-I.', '-ansi', '-Wall'])
#env.Append(CXXFLAGS = commands.getoutput('pkg-config sigc++-2.0 --cflags').split())
env.Prepend(CXXFLAGS = commands.getoutput('pkg-config glib-2.0 --cflags').split())
#env.Append(_LIBFLAGS = ' ' + commands.getoutput('pkg-config sigc++-2.0 --libs'))
env.Append(_LIBFLAGS = ' ' + commands.getoutput('pkg-config glib-2.0 --libs'))

env['build_path'] = 'build/'
if int(release):
	env['build_path'] = 'build/release/'
elif int(debug):
	env['build_path'] = 'build/debug/'

env.Append(LIBPATH = env['build_path'])
	

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

