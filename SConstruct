import os

def AddPkg(self, pkg):
	self.ParseConfig('pkg-config --cflags --libs \'%s\'' % pkg)

AddMethod(Environment, AddPkg)

env = Environment(ENV = {'PATH' : os.environ['PATH']})

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

	RequiredPkgs = [
				'libpmemobj++',
				'jsoncpp',
	]
	RequiredLibs = [
				'boost_system', 
				'boost_program_options', 
				'boost_filesystem', 
				'boost_unit_test_framework',
				'pthread', 
				'rt', 
				'dl',
				'log4cxx',
				'libpmem',
				'fabric',
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
SConscript('tests/SConscript', exports=['env', ])

config.Finish()
