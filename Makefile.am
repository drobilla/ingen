SUBDIRS = src

doc: Doxyfile src/engine/*.h src/engine/*.cpp \
src/common/util/*.h \
src/common/interface/*.h \
src/clients/*.h src/clients/*.cpp \
src/clients/gtk/*.h src/clients/gtk/*.cpp
	doxygen
