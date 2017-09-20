env = Environment(tools = ['default', 'Make'])
env.Append(CCFLAGS = ['-std=c++11', '-O0', '-ggdb'])

VariantDir('build', '.')
SConscript('build/src/SConscript', exports=['env',])