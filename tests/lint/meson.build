# Copyright 2019-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

if not meson.is_subproject()
  # Check code with cppcheck
  cppcheck = find_program('cppcheck', required: false)
  if cppcheck.found()
    compdb_path = join_paths(ingen_build_root, 'compile_commands.json')
    suppress_path = join_paths(ingen_src_root, '.suppress.cppcheck')
    test(
      'cppcheck',
      cppcheck,
      args: [
        '--cppcheck-build-dir=' + meson.current_build_dir(),
        '--enable=warning,style,performance,portability',
        '--error-exitcode=1',
        '--inline-suppr',
        '--project=' + compdb_path,
        '--suppressions-list=' + suppress_path,
        '-q',
      ],
      suite: 'code',
    )
  endif
endif
