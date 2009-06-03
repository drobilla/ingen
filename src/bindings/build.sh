#!/bin/sh
swig -c++ -python ./ingen.i

gcc -shared -o _ingen.so ./ingen_wrap.cxx ./ingen_bindings.cpp -I.. -I../common `pkg-config --cflags --libs glibmm-2.4` `python-config --cflags --ldflags` -fPIC
