# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR GPL-3.0-or-later

png_icon_sizes = [
  '16x16',
  '22x22',
  '24x24',
  '32x32',
  '48x48',
  '64x64',
  '128x128',
  '256x256',
]

icons_dir = get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor'

install_data('scalable/ingen.svg',
             install_dir: icons_dir / 'scalable' / 'apps')

foreach size : png_icon_sizes
  install_data(files(size / 'ingen.png'),
               install_dir: icons_dir / size / 'apps')
endforeach
