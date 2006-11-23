SUBDIRS = src patches

doc: doc/Doxyfile src/libs/engine/*.h src/libs/engine/*.cpp \
src/common/interface/*.h \
src/libs/client/*.h src/libs/client/*.cpp \
src/progs/ingenuity/*.h src/progs/ingenuity/*.cpp
	doxygen doc/Doxyfile
