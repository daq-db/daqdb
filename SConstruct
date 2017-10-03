env = Environment(tools = ['default', 'Make'])
env.Append(CCFLAGS = ['-std=c++11', '-O0', '-ggdb'])
env.Append(VERBOSE = ARGUMENTS.get('verbose', 0))

# todo lib dependencies for fedora and ubuntu - need more generic mechanism
env.Append(LIBPATH=["/usr/lib64", "/usr/lib/x86_64-linux-gnu", "#lib","#src/3rd/cChord","#build/src/dht"])

VariantDir('build', '.')
SConscript('build/src/SConscript', exports=['env',])