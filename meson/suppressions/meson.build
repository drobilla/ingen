# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#######
# C++ #
#######

if is_variable('cpp')
  cpp_suppressions = []

  if get_option('strict')
    if cpp.get_id() == 'clang'
      cpp_suppressions = [
        '-Wno-c++17-extensions',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-documentation-unknown-command',
        '-Wno-exit-time-destructors',
        '-Wno-float-conversion',
        '-Wno-float-equal',
        '-Wno-format-nonliteral',
        '-Wno-global-constructors',
        '-Wno-implicit-float-conversion',
        '-Wno-implicit-int-conversion',
        '-Wno-nullability-extension',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-shadow-field',
        '-Wno-shorten-64-to-32',
        '-Wno-sign-conversion',
        '-Wno-switch-enum',
        '-Wno-unreachable-code',
        '-Wno-unused-parameter',
        '-Wno-vla',
        '-Wno-vla-extension',
        '-Wno-weak-vtables',
      ]

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

    elif cpp.get_id() == 'gcc'
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
        '-Wno-unused-parameter',
        '-Wno-useless-cast',
        '-Wno-vla',
      ]
    endif
  endif

  cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
endif
