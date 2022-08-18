# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR GPL-3.0-or-later

data_files = files(
  'MonoEffect.ttl',
  'MonoInstrument.ttl',
  'StereoEffect.ttl',
  'StereoInstrument.ttl',
  'errors.ttl',
  'ingen.ttl',
  'internals.ttl',
  'manifest.ttl',
)

# Install bundle
install_data(data_files, install_dir: lv2dir / 'ingen.lv2')

# Ontology documentation
lv2specgen_py = find_program('lv2specgen.py', required: get_option('docs'))
if lv2specgen_py.found()
  ingen_html = custom_target(
    'ingen.html',
    input: files('ingen.ttl'),
    command: [
      lv2specgen_py,
      '@INPUT@',
      '@OUTPUT@',
      '--copy-style',
      '--instances',
      '--list-email', 'ingen@drobilla.net',
      '--list-page', 'http://lists.drobilla.net/listinfo.cgi/ingen-drobilla.net',
      '--prefix', 'ingen',
    ],
    install: true,
    install_dir: lv2dir / 'ingen.lv2',
    output: 'ingen.html',
  )

  # TODO: Fix lv2specgen so third-party documentation is properly styled
  install_data(
    files('style.css'),
    install_dir: lv2dir / 'ingen.lv2',
  )
endif
