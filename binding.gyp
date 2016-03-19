{
  'targets': [
    {
      'target_name': 'speaky',
      'sources': [
        'src/binding.cc',
      ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")"
      ],
      'libraries': [
        '-lttspico'
      ],
      'cflags': [ '-O3', '-std=c++0x' ],
      'conditions': [
        [ 'OS=="mac"', {
          'xcode_settings': {
            'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
            'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++0x',  # -std=gnu++0x
          },
        }],
      ],
    },
  ],
}
