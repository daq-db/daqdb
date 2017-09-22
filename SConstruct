env = Environment(tools = ['default', 'Make'])
env.Append(CCFLAGS = ['-std=c++11', '-O0', '-ggdb'])
env.Append(VERBOSE = ARGUMENTS.get('verbose', 0))

VariantDir('build', '.')
SConscript('build/src/SConscript', exports=['env',])