# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

scripts = files(
  'ingenams',
  'ingenish',
)

foreach script : scripts
  install_data(
    script,
    install_dir: get_option('bindir'),
    install_mode: 'rwxr-xr-x',
  )
endforeach

pymod = import('python')
py = pymod.find_installation('python3', required: get_option('bindings_py'))

if py.found()
  py.install_sources(files('ingen.py'))
endif
