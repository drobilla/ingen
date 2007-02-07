#!/bin/sh

echo 'Generating necessary files...'
libtoolize --copy --force
aclocal-1.9
autoheader -Wall
automake-1.9 --gnu --add-missing
autoconf

