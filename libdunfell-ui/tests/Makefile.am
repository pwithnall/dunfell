include $(top_srcdir)/glib-tap.mk

installed_testdir = $(libexecdir)/installed-tests/dunfell-ui-@DWL_API_VERSION@
installed_test_metadir = $(datadir)/installed-tests/dunfell-ui-@DWL_API_VERSION@

# Flags for all test binaries
AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/libdunfell-ui \
	-DTEST_FILE_DIR="\"$(top_srcdir)/libdunfell-ui/tests/\"" \
	-DG_LOG_DOMAIN=\"libdunfell-ui-tests\" \
	-DLIBDUNFELL_UI_DISABLE_DEPRECATED \
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
	$(top_builddir)/libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la \
	$(GLIB_LIBS) \
	$(NULL)

@VALGRIND_CHECK_RULES@

test_programs = \
	$(NULL)

-include $(top_srcdir)/git.mk
