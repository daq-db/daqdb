import os

env = Environment()

env.Append(CCFLAGS=['-std=c++11', '-O0', '-ggdb'])
env.Append(LIBPATH=['/usr/lib64', '/usr/lib/x86_64-linux-gnu', '#third-party/cChord', '#build/src/dht', '#third-party/pmemkv/bin'])
env.Append(LINKFLAGS='-Wl,-rpath=.')  # loads shared library from the binary directory

env.Append(VERBOSE=ARGUMENTS.get('verbose', 0))

SConscript('third-party/SConscript', exports='env')
VariantDir('build', 'lib', duplicate=0) # to not trash lib folder
SConscript('build/SConscript', exports=['env', ])
SConscript('examples/node/SConscript', exports=['env', ])
SConscript('tests/SConscript', exports=['env', ])

# copy products to bin directory
instDragon = env.Install('bin', 'examples/node/dragon')
instDragonTest = env.Install('bin', 'tests/dragonTest')
env.Alias('install', [instDragon, instDragonTest])
Depends('install', 'build')

if not GetOption("clean"):
	RequiredLibs = [
				'boost_system', 
				'boost_program_options', 
				'boost_filesystem', 
				'boost_unit_test_framework',
				'pthread', 
				'rt', 
				'dl',
				'log4cxx'
				]

	config = Configure(env)
	for required_lib in RequiredLibs:
		if not config.CheckLib(required_lib):
			config.Finish()
			exit(1)
	config.Finish()