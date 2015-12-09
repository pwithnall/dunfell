include $(top_srcdir)/gtester.mk

# Test environment.
TESTS_ENVIRONMENT = \
	G_TEST_SRCDIR="$(abs_srcdir)" \
	G_TEST_BUILDDIR="$(abs_builddir)" \
	$(NULL)

# Test programs.
test_cppflags = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/libdunfell \
	-DTEST_FILE_DIR="\"$(top_srcdir)/libdunfell/tests/\"" \
	-DG_LOG_DOMAIN=\"libdunfell-tests\" \
	-DLIBDUNFELL_DISABLE_DEPRECATED \
	$(DISABLE_DEPRECATED) \
	$(NULL)
test_cflags = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(NULL)
test_ldflags = \
	$(WARN_LDFLAGS) \
	$(NULL)
test_ldadd = \
	$(top_builddir)/libdunfell/libdunfell-@DFL_API_VERSION@.la \
	$(GLIB_LIBS) \
	$(NULL)

@VALGRIND_CHECK_RULES@

# TODO: installed-tests
TESTS = \
	$(NULL)
noinst_PROGRAMS = $(TESTS)

#schema_CPPFLAGS = $(test_cppflags)
#schema_CFLAGS = $(test_cflags)
#schema_LDFLAGS = $(test_ldflags)
#schema_LDADD = $(test_ldadd)

-include $(top_srcdir)/git.mk
