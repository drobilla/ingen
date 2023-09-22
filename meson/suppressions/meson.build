# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

# Project-specific warning suppressions

warning_level = get_option('warning_level')

#######
# C++ #
#######

if is_variable('cpp')
  cpp_suppressions = []

  if cpp.get_id() == 'clang'
    if warning_level == 'everything'
      cpp_suppressions = [
        '-Wno-c++17-extensions',
        '-Wno-c++98-compat',
        '-Wno-c++98-compat-pedantic',
        '-Wno-cast-align',
        '-Wno-cast-function-type-strict',
        '-Wno-cast-qual',
        '-Wno-documentation-unknown-command',
        '-Wno-exit-time-destructors',
        '-Wno-float-conversion',
        '-Wno-float-equal',
        '-Wno-format-nonliteral',
        '-Wno-global-constructors',
        '-Wno-implicit-float-conversion',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-shorten-64-to-32',
        '-Wno-sign-conversion',
        '-Wno-switch-enum',
        '-Wno-unreachable-code',
        '-Wno-unsafe-buffer-usage',
        '-Wno-vla',
        '-Wno-weak-vtables',
      ]

      if not meson.is_cross_build()
        cpp_suppressions += [
          '-Wno-poison-system-directories',
        ]
      endif

      if host_machine.system() in ['darwin', 'freebsd']
        cpp_suppressions += [
          '-Wno-comma', # boost
          '-Wno-deprecated-copy', # boost
          '-Wno-disabled-macro-expansion', # boost
          '-Wno-documentation', # JACK
          '-Wno-documentation-deprecated-sync', # JACK
          '-Wno-extra-semi-stmt', # boost
          '-Wno-old-style-cast', # boost
          '-Wno-redundant-parens', # boost
          '-Wno-suggest-destructor-override', # boost
          '-Wno-suggest-override', # boost
          '-Wno-unused-template', # boost
          '-Wno-zero-as-null-pointer-constant', # boost
        ]
      endif
    endif

    if warning_level in ['everything', '3']
      cpp_suppressions += [
        '-Wno-unused-parameter',
        '-Wno-vla-extension',
        '-Wno-nullability-extension',
      ]
    endif

  elif cpp.get_id() == 'gcc'
    if warning_level == 'everything'
      cpp_suppressions = [
        '-Wno-abi-tag',
        '-Wno-alloc-zero',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-conditionally-supported',
        '-Wno-conversion',
        '-Wno-effc++',
        '-Wno-float-conversion',
        '-Wno-float-equal',
        '-Wno-format',
        '-Wno-format-nonliteral',
        '-Wno-format-truncation',
        '-Wno-inline',
        '-Wno-multiple-inheritance',
        '-Wno-null-dereference',
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-redundant-tags',
        '-Wno-sign-conversion',
        '-Wno-stack-protector',
        '-Wno-strict-overflow',
        '-Wno-suggest-attribute=const',
        '-Wno-suggest-attribute=format',
        '-Wno-suggest-attribute=pure',
        '-Wno-suggest-final-methods',
        '-Wno-suggest-final-types',
        '-Wno-suggest-override',
        '-Wno-switch-default',
        '-Wno-switch-enum',
        '-Wno-unreachable-code',
        '-Wno-unused-const-variable',
        '-Wno-useless-cast',
      ]
    endif

    if warning_level in ['everything', '3']
      cpp_suppressions += [
        '-Wno-unused-parameter',
        '-Wno-vla',
      ]
    endif
  endif

  cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
endif
