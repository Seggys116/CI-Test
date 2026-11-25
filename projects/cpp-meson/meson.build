project(
  'ciutil',
  'cpp',
  version: '1.0.0',
  meson_version: '>= 1.3.0',
  default_options: ['cpp_std=c++17', 'warning_level=3', 'werror=true'],
)

cxx = meson.get_compiler('cpp')

# sanitizers are opt-in: -Dsanitize=enabled selects the flavour from -Dsanitizers.
# the stock b_sanitize option works as well and is independent of this one.
if not get_option('sanitize').disabled() and get_option('sanitizers') != 'none'
  sanitizer_flag = '-fsanitize=' + get_option('sanitizers')
  if cxx.has_argument(sanitizer_flag) and cxx.has_link_argument(sanitizer_flag)
    sanitizer_args = [sanitizer_flag, '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    add_project_arguments(sanitizer_args, language: 'cpp')
    add_project_link_arguments(sanitizer_args, language: 'cpp')
  elif get_option('sanitize').enabled()
    error('sanitizers requested but ' + cxx.get_id() + ' does not support ' + sanitizer_flag)
  endif
endif

includes = include_directories('include')

ciutil_lib = static_library(
  'ciutil',
  files(
    'src/adler32.cpp',
    'src/base64.cpp',
    'src/cli.cpp',
    'src/crc32.cpp',
    'src/roman.cpp',
    'src/semver.cpp',
  ),
  include_directories: includes,
  install: true,
)

ciutil_dep = declare_dependency(
  link_with: ciutil_lib,
  include_directories: includes,
)

ciutil_exe = executable(
  'ciutil',
  'src/main.cpp',
  dependencies: ciutil_dep,
  install: true,
)

install_subdir('include/ciutil', install_dir: get_option('includedir'))

pkg = import('pkgconfig')
pkg.generate(
  ciutil_lib,
  name: 'ciutil',
  description: 'small checksum and codec utilities',
  version: meson.project_version(),
)

meson.override_dependency('ciutil', ciutil_dep)

subdir('tests')
