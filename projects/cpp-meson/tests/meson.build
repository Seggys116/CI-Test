test_modules = ['crc32', 'adler32', 'base64', 'roman', 'semver', 'cli']

foreach module : test_modules
  test_exe = executable(
    'test_' + module,
    module + '_test.cpp',
    dependencies: ciutil_dep,
  )
  test(module, test_exe, suite: 'unit')
endforeach

# the real binary, not just the run() function: must start and exit 0
test('cli-version', ciutil_exe, args: ['version'], suite: 'cli')
