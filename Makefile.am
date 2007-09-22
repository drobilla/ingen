SUBDIRS = src patches

doc: doc/Doxyfile \
src/common/interface/*.hpp \
src/libs/engine/*.hpp src/libs/engine/*.cpp \
src/libs/client/*.hpp src/libs/client/*.cpp \
src/libs/gui/*.hpp src/libs/gui/*.cpp \
src/libs/serialisation/*.hpp src/libs/serialisation/*.cpp
	doxygen doc/Doxyfile
