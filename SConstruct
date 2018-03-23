import os
import subprocess
from distutils.spawn import find_executable

def AddPkg(self, pkg):
	self.ParseConfig('pkg-config --cflags --libs \'%s\'' % pkg)

class SPDKConfig:
	def __init__(self, mk):
		self.Makefile = mk;
		self.libs = list();
		self.__parse_all();

	def env_append(self, env):
		env.Append(CPPFLAGS = self.CPPPATH)
		env.Append(LINKFLAGS = self.LINKFLAGS)

	def add_lib(self, lib):
		self.libs.extend(lib);
		self.__parse_all();

	def __parse_all(self):
		self.LINKFLAGS = self.__parse("LIBS") + self.__parse("SYS_LIBS") +  self.__parse("LDFLAGS");
		self.CPPPATH = self.__parse("CPPPATH");
	
	def __parse(self, target):
		cmd = ["make", "-s", "-f", self.Makefile, target];
		if self.libs:
			cmd = cmd + ["SPDK_LIB_LIST=\"{0}\"".format(" ".join(self.libs))];
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
		(output, err) = process.communicate()
		exit_code = process.wait()
		if exit_code:
			raise Exception("make failed");
		return output.split();

AddMethod(Environment, AddPkg)

'''
	Scons input arguments
'''
AddOption('--lcg', dest='use_lcg_env', action='store_true',
		  help='Use CERN LCG environment')
AddOption('--doc', dest='gen_doxygen', action='store_true',
		  help='Generate doxygen')
AddOption('--verbose', dest='verbose', action='store_true',
		  help='Prints all test messages')

env = Environment(ENV = os.environ)

env.spdk = SPDKConfig(Dir("#third-party").abspath + "/spdk.mk");

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
env.Append(CCFLAGS = ['-O0'])
env.Append(CXXFLAGS = ['-g3', '-std=c++14'])
env.Append(CPPFLAGS = [])

'''
	Paths for LCG
'''
if GetOption('use_lcg_env'):
	env.Append(CPPPATH = [p + '/include' for p in env['ENV']['CMAKE_PREFIX_PATH'].split(':')])
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

config = Configure(env, custom_tests = {'CheckPkg': CheckPkg, 'CheckPkgConfig': CheckPkgConfig})

if not GetOption("clean"):
	if not config.CheckCXXHeader('boost/hana.hpp'):
		env.Append(CPPFLAGS = ['-DHAS_BOOST_HANA=0'])
	else:
		env.Append(CPPFLAGS = ['-DHAS_BOOST_HANA=1'])

	if config.CheckLib('log4cxx'):
		env.Append(CPPFLAGS = ['-DFOGKV_USE_LOG4CXX'])

	if not config.CheckCXXHeader('json/json.h'):
		p = subprocess.Popen(["pkg-config", "--variable=includedir", 'jsoncpp'], stdout=subprocess.PIPE)
		includedir, err = p.communicate()
		if not err:
			env.Append(CPPPATH = Dir(includedir.strip()))

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
		'dl'
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
if not env.GetOption('clean'):
	SConscript('third-party/SConscript', exports='env')
else:
	if not ('lib' in COMMAND_LINE_TARGETS):
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
if not ('lib' in COMMAND_LINE_TARGETS):
	SConscript('examples/SConscript', exports=['env', ])

'''
	Generate doxygen
'''
if GetOption('gen_doxygen'):
	if find_executable("doxygen") is not None:
		genDoxygen = env.Command('doxygen', "", 'cd %s && doxygen fogkv.Doxyfile' % Dir('#').abspath)
		env.Alias('doc', genDoxygen)
	else:
		print ("Please install doxygen to generate documentation")

'''
	Generate doxygen
'''
if GetOption('gen_doxygen'):
	if find_executable("doxygen") is not None:
		genDoxygen = env.Command('doxygen', "", 'cd %s && doxygen fogkv.Doxyfile' % Dir('#').abspath)
		env.Alias('doc', genDoxygen)
	else:
		print ("Please install doxygen to generate documentation")

env.Alias('install', '#bin')

Depends('install', 'build')

'''
	Build and execute tests
'''
if 'tests' in COMMAND_LINE_TARGETS:
	SConscript('tests/SConscript', exports=['env', ])

if env.GetOption('clean'):
	env.Clean('bin', Dir('#bin'))

	if 'distclean' in COMMAND_LINE_TARGETS:
		env.Clean("distclean",[
			".sconsign.dblite",
			".sconf_temp",
			"config.log",
			])
