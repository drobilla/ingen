#!/usr/bin/env sh

# Source this file (e.g ". set_dev_environment.sh") to set up a development
# environment so dynamic modules in the source tree will be found and the
# executables can be run directly, or in gdb/valgrind/etc.

export INGEN_MODULE_PATH="`pwd`/libs/engine/.libs:`pwd`/libs/serialisation/.libs:`pwd`/libs/gui/.libs:`pwd`/libs/client/.libs"

