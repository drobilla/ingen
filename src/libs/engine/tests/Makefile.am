if BUILD_UNIT_TESTS

AM_CXXFLAGS = @JACK_CFLAGS@ @LIBLO_CFLAGS@ @ALSA_CFLAGS@ -I$(top_srcdir)/raul -I../../common
common_ldadd = @JACK_LIBS@ @RAUL_LIBS@ @LIBLO_LIBS@ @ALSA_LIBS@ -lrt
node_tree_test_LDADD = $(common_ldadd)

noinst_PROGRAMS = node_tree_test

node_tree_test_SOURCES = \
	node_tree_test.cpp

endif # BUILD_UNIT_TESTS
