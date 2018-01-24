import os

def AddPkg(self, pkg):
	self.ParseConfig('pkg-config --cflags --libs \'%s\'' % pkg)

AddMethod(Environment, AddPkg)

AddOption('--lcg', dest='use_lcg_env', action='store_true',
		  help='Use CERN LCG environment')

env = Environment(ENV = os.environ)

def CheckPkgConfig(ctx, version = None):
	if version:
		ctx.Message('Checking for pkg-config version %s...' % version)
		ret = ctx.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	else:
		ctx.Message('Checking for pkg-config...')
		ret = ctx.TryAction('pkg-config --version')[0]
	ctx.Result(ret)
	return ret;
	
		

def CheckPkg(ctx, pkg):
	ctx.Message('Checking for %s package using pkg-config...' % pkg)
	ret = ctx.TryAction('pkg-config --exists \'%s\'' % pkg)[0]
	ctx.Result(ret);
	return ret

'''
	Compiler flags
'''
env.Append(CCFLAGS = ['-O0', '-ggdb'])
env.Append(CXXFLAGS = ['-std=c++14'])
env.Append(CPPFLAGS = [])

'''
	Paths for LCG
'''
if GetOption('use_lcg_env'):
	env.Append(CPPPATH = [env['ENV']['CMAKE_PREFIX_PATH'] + '/include'])
	env.Append(LIBPATH = env['ENV']['LD_LIBRARY_PATH'].split(':'))

'''
	Depends on linux distribution
'''
env.Append(LIBPATH=[
	'/usr/lib64',					# Fedora
	'/usr/lib/x86_64-linux-gnu', 	# Ubuntu
	'#build/', 
	])

'''
	Linker options
	- loads shared library from the binary directory
'''
env.Append(LINKFLAGS='-Wl,-rpath=.')

env.Append(CPPPATH=[Dir('#include')])

env.Append(VERBOSE=ARGUMENTS.get('verbose', 0))

config = Configure(env, custom_tests = {'CheckPkg': CheckPkg, 'CheckPkgConfig': CheckPkgConfig})

if not GetOption("clean"):

	if not config.CheckCXXHeader('boost/hana.hpp'):
		env.Append(CPPFLAGS = ['-DHAS_BOOST_HANA=0'])
	else:
		env.Append(CPPFLAGS = ['-DHAS_BOOST_HANA=1'])

	if config.CheckLib('log4cxx'):
		env.Append(CPPFLAGS = ['-DFOGKV_USE_LOG4CXX'])

	RequiredPkgs = [
	]
	RequiredLibs = [
				'jsoncpp',
				'boost_system', 
				'boost_program_options', 
				'boost_filesystem', 
				'boost_unit_test_framework',
				'pthread', 
				'rt', 
				'dl',
	]


	for required_lib in RequiredLibs:
		if not config.CheckLib(required_lib):
			config.Finish()
			exit(1)

	if not config.CheckPkgConfig():
		config.Finish()
		exit(1)

	for required_pkg in RequiredPkgs:
		if not config.CheckPkg(required_pkg):
			config.Finish()
			exit(1)
		else:
			env.AddPkg(required_pkg)


'''
	Build dependencies first
'''
SConscript('third-party/SConscript', exports='env')

'''
	Build FogKV library (shared, static ones)
	Products will be saved in "build" directory to not trash lib folder
'''
VariantDir('build', 'lib', duplicate=0)
SConscript('build/SConscript', exports=['env', ])

'''
	Build examples
'''
SConscript('examples/SConscript', exports=['env', ])

env.Alias('install', '#bin')
Depends('install', 'build')

'''
	Build and execute tests
'''
#todo @gjerecze fix for lcg
SConscript('tests/SConscript', exports=['env', ])
