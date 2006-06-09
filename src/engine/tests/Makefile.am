if BUILD_UNIT_TESTS

AM_CXXFLAGS = @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ -I../../common
common_ldadd = @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ -lrt
node_tree_test_LDADD = $(common_ldadd)
queue_test_LDADD = $(common_ldadd)

bin_PROGRAMS = node_tree_test queue_test list_test path_test

list_test_SOURCES = \
	../List.h \
	list_test.cpp

path_test_SOURCES = \
	../../common/Path.h \
	path_test.cpp

node_tree_test_SOURCES = \
	node_tree_test.cpp \
	../Tree.h \
	../TreeImplementation.h

queue_test_SOURCES = \
	queue_test.cpp \
	../../common/Queue.h

endif # BUILD_UNIT_TESTS
