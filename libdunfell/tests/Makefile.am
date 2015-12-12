include $(top_srcdir)/glib-tap.mk

installed_testdir = $(libexecdir)/installed-tests/dunfell-@DFL_API_VERSION@
installed_test_metadir = $(datadir)/installed-tests/dunfell-@DFL_API_VERSION@

# Flags for all test binaries
AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/libdunfell \
	-DTEST_FILE_DIR="\"$(top_srcdir)/libdunfell/tests/\"" \
	-DG_LOG_DOMAIN=\"libdunfell-tests\" \
	-DLIBDUNFELL_DISABLE_DEPRECATED \
	$(DISABLE_DEPRECATED) \
	$(NULL)
AM_CFLAGS = \
	$(WARN_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(NULL)
AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(NULL)
LDADD = \
	$(top_builddir)/libdunfell/libdunfell-@DFL_API_VERSION@.la \
	$(GLIB_LIBS) \
	$(NULL)

@VALGRIND_CHECK_RULES@

test_programs = \
	event-sequence \
	main-context \
	parser \
	$(NULL)

-include $(top_srcdir)/git.mk
