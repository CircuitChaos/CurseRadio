import subprocess

def getGitCommand(cmd):
    return subprocess.Popen('git ' + cmd, stdout=subprocess.PIPE, shell=True).stdout.read().decode('utf-8').strip()

def getGitHash():
    return getGitCommand('rev-parse --short HEAD')

env = Environment()
env['CCFLAGS']	= '-Wall -Wextra -std=c++17 -O2 -march=native -g -DGIT_HASH=' + getGitHash()
env['CPPPATH']	= 'src'
env['LIBS'] = 'ncurses'

env.VariantDir('build', 'src', duplicate = 0)
env.AlwaysBuild(['build/version.o', 'build/curseradio'])
curseradio = env.Program('build/curseradio', Glob('build/*.cpp'))

env.Install('/usr/local/bin', curseradio)
env.Alias('install', '/usr/local/bin')
