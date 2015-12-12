SUBDIRS = . libdunfell/tests libdunfell-ui/tests libdunfell/docs libdunfell-ui/docs

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST =
bin_PROGRAMS =
bin_SCRIPTS =
man8_MANS =
VAPIGEN_VAPIS =
lib_LTLIBRARIES =

# libdunfell library
lib_LTLIBRARIES += libdunfell/libdunfell-@DFL_API_VERSION@.la

dflincludedir = $(includedir)/libdunfell-@DFL_API_VERSION@/dunfell
dfl_headers = \
	libdunfell/dfl-event.h \
	libdunfell/dfl-event-sequence.h \
	libdunfell/dfl-main-context.h \
	libdunfell/dfl-parser.h \
	libdunfell/dfl-thread.h \
	libdunfell/dfl-types.h \
	libdunfell/dfl-version.h \
	$(NULL)

# The following headers are private, and shouldn't be installed:
dfl_private_headers = \
	libdunfell/dfl-time-sequence.h \
	$(NULL)
dflinclude_HEADERS = \
	$(dfl_main_header) \
	$(dfl_headers) \
	$(NULL)

dfl_sources = \
	libdunfell/dfl-event.c \
	libdunfell/dfl-event-sequence.c \
	libdunfell/dfl-main-context.c \
	libdunfell/dfl-parser.c \
	libdunfell/dfl-thread.c \
	libdunfell/dfl-time-sequence.c \
	$(NULL)

dfl_main_header = libdunfell/dunfell.h
dfl_public_headers = $(dflinclude_HEADERS)

libdunfell_libdunfell_@DFL_API_VERSION@_la_SOURCES = \
	$(dfl_private_headers) \
	$(dfl_sources) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libdunfell \
	-I$(top_builddir)/libdunfell \
	-DG_LOG_DOMAIN=\"libdunfell\" \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_LIBADD = \
	$(GLIB_LIBS) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(WARN_LDFLAGS) \
	$(AM_LIBADD) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_LDFLAGS = \
	-version-info $(DFL_LT_VERSION) \
	-export-symbols-regex "^dfl_" \
	-no-undefined \
	$(AM_LDFLAGS) \
	$(NULL)

# Check if dunfell.h includes all the public headers
check-local: check-headers
check-headers:
	@any_missing=0; for header_file in $(dfl_public_headers); do \
		if test "x$$header_file" != "x$(dfl_main_header)"; then \
			if ! grep "#include <$${header_file#lib}>" $(top_srcdir)/$(dfl_main_header) >/dev/null; then \
				echo "$(dfl_main_header) doesn't appear to include \"$$header_file\""; \
				any_missing=1; \
			fi; \
		fi; \
	done; exit "$$any_missing"

# libdunfell-ui library
lib_LTLIBRARIES += libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la

dwlincludedir = $(includedir)/libdunfell-ui-@DWL_API_VERSION@/dunfell-ui
dwl_headers = \
	libdunfell-ui/dwl-timeline.h \
	libdunfell-ui/dwl-version.h \
	$(NULL)

# The following headers are private, and shouldn't be installed:
dwl_private_headers = \
	$(NULL)
dwlinclude_HEADERS = \
	$(dwl_main_header) \
	$(dwl_headers) \
	$(NULL)

dwl_sources = \
	libdunfell-ui/dwl-timeline.c \
	$(NULL)

dwl_main_header = libdunfell-ui/dunfell-ui.h
dwl_public_headers = $(dwlinclude_HEADERS)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_SOURCES = \
	$(dwl_private_headers) \
	$(dwl_sources) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libdunfell-ui \
	-I$(top_builddir)/libdunfell-ui \
	-DG_LOG_DOMAIN=\"libdunfell-ui\" \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(WARN_LDFLAGS) \
	$(AM_LIBADD) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_LDFLAGS = \
	-version-info $(DWL_LT_VERSION) \
	-export-symbols-regex "^dwl_" \
	-no-undefined \
	$(AM_LDFLAGS) \
	$(NULL)

# Check if dunfell-ui.h includes all the public headers
check-local: check-dwl-headers
check-dwl-headers:
	@any_missing=0; for header_file in $(dwl_public_headers); do \
		if test "x$$header_file" != "x$(dwl_main_header)"; then \
			if ! grep "#include <$${header_file#lib}>" $(top_srcdir)/$(dwl_main_header) >/dev/null; then \
				echo "$(dwl_main_header) doesn't appear to include \"$$header_file\""; \
				any_missing=1; \
			fi; \
		fi; \
	done; exit "$$any_missing"

.PHONY: check-dwl-headers

# dunfell-record program
dfldatadir = $(datadir)/libdunfell-@DFL_API_VERSION@
dist_dfldata_DATA = agent/dunfell-record.stp

EXTRA_DIST += agent/dunfell-record.in
bin_SCRIPTS += agent/dunfell-record
CLEANFILES += agent/dunfell-record

agent/dunfell-record: $(srcdir)/agent/dunfell-record.in
	$(AM_V_GEN)sed -e "s,[@]datadir[@],$(datadir),g;s,[@]DFL_API_VERSION[@],@DFL_API_VERSION@,g" $< > $@ || rm $@

# Macros
EXTRA_DIST += \
	m4/ax_append_compile_flags.m4 \
	m4/ax_append_flag.m4 \
	m4/ax_check_compile_flag.m4 \
	m4/ax_check_enable_debug.m4 \
	m4/ax_code_coverage.m4 \
	m4/ax_compiler_flags.m4 \
	m4/ax_pkg_check_modules.m4 \
	m4/ax_require_defined.m4 \
	m4/ax_valgrind_check.m4 \
	$(NULL)

# Introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =
DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
EXTRA_DIST += m4/introspection.m4

if HAVE_INTROSPECTION
libdunfell/Dunfell-@DFL_API_VERSION@.gir: libdunfell/libdunfell-@DFL_API_VERSION@.la
libdunfell_Dunfell_@DFL_API_VERSION@_gir_INCLUDES = GObject-2.0 Gio-2.0
libdunfell_Dunfell_@DFL_API_VERSION@_gir_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(libdunfell_libdunfell_@DFL_API_VERSION@_la_CPPFLAGS) \
	$(NULL)
libdunfell_Dunfell_@DFL_API_VERSION@_gir_LIBS = libdunfell/libdunfell-@DFL_API_VERSION@.la
libdunfell_Dunfell_@DFL_API_VERSION@_gir_FILES = $(dfl_sources) $(dfl_headers)
libdunfell_Dunfell_@DFL_API_VERSION@_gir_NAMESPACE = Dunfell
libdunfell_Dunfell_@DFL_API_VERSION@_gir_EXPORT_PACKAGES = libdunfell
libdunfell_Dunfell_@DFL_API_VERSION@_gir_SCANNERFLAGS = \
	--nsversion=@DFL_API_VERSION@ \
	--identifier-prefix=Dfl \
	--symbol-prefix=dfl \
	--c-include="libdunfell/dunfell.h" \
	--warn-all \
	--warn-error \
	$(NULL)

INTROSPECTION_GIRS += libdunfell/Dunfell-@DFL_API_VERSION@.gir

libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir: libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_INCLUDES = GObject-2.0 Gio-2.0 Gtk-3.0
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_CPPFLAGS) \
	$(NULL)
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_LIBS = libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_FILES = $(dwl_sources) $(dwl_headers)
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_NAMESPACE = DunfellUi
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_EXPORT_PACKAGES = libdunfell-ui
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_SCANNERFLAGS = \
	--nsversion=@DWL_API_VERSION@ \
	--identifier-prefix=Dwl \
	--symbol-prefix=dwl \
	--c-include="libdunfell-ui/dunfell-ui.h" \
	--warn-all \
	--warn-error \
	$(NULL)

INTROSPECTION_GIRS += libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

if ENABLE_VAPIGEN
-include $(VAPIGEN_MAKEFILE)

# Note: The VAPI file name has to match the pkg-config file name.
libdunfell/libdunfell-@DFL_API_VERSION@.vapi: libdunfell/Dunfell-@DFL_API_VERSION@.gir

VAPIGEN_VAPIS += libdunfell/libdunfell-@DFL_API_VERSION@.vapi

libdunfell_libdunfell_@DFL_API_VERSION@_vapi_DEPS =
libdunfell_libdunfell_@DFL_API_VERSION@_vapi_METADATADIRS = $(srcdir)/libdunfell
libdunfell_libdunfell_@DFL_API_VERSION@_vapi_FILES = libdunfell/Dunfell-@DFL_API_VERSION@.gir

libdunfell/libdunfell-@DFL_API_VERSION@.deps:
	$(AM_V_GEN) for pkg in $(libdunfell_libdunfell_@DFL_API_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

# Note: The VAPI file name has to match the pkg-config file name.
libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.vapi: libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir

VAPIGEN_VAPIS += libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.vapi

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_DEPS = gtk+-3.0
libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_METADATADIRS = $(srcdir)/libdunfell-ui
libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_FILES = libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir

libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.deps:
	$(AM_V_GEN) for pkg in $(libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)

CLEANFILES += $(vapi_DATA)
endif
endif

# Code coverage
@CODE_COVERAGE_RULES@
CODE_COVERAGE_DIRECTORY = $(top_builddir)/libdunfell $(top_builddir)/libdunfell-ui
CODE_COVERAGE_LCOV_OPTIONS = --base-directory $(abs_top_srcdir)

# Cleaning
EXTRA_DIST += \
	autogen.sh \
	HACKING \
	gtk-doc.make \
	dunfell.doap \
	glib-tap.mk \
	tap-driver.sh \
	tap-test \
	$(NULL)

CLEANFILES += \
	`find "$(srcdir)" -type f -name .dirstamp -print` \
	$(NULL)

DISTCLEANFILES = \
	gtk-doc.make \
	$(NULL)

MAINTAINERCLEANFILES += \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	build-aux/ \
	INSTALL \
	omf.make \
	gtk-doc.make \
	config.rpath \
	m4/gtk-doc.m4 \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc

# pkg-config data
# Note that the template file is called libdunfell.pc.in, but generates a
# versioned .pc file using some magic in AC_CONFIG_FILES.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	libdunfell/libdunfell-$(DFL_API_VERSION).pc \
	libdunfell-ui/libdunfell-ui-$(DWL_API_VERSION).pc \
	$(NULL)

DISTCLEANFILES += $(pkgconfig_DATA)
EXTRA_DIST += \
	libdunfell/libdunfell.pc.in \
	libdunfell/dfl-version.h.in \
	libdunfell-ui/libdunfell-ui.pc.in \
	libdunfell-ui/dwl-version.h.in \
	$(NULL)

# ChangeLog
ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog

MAINTAINERCLEANFILES += ChangeLog

-include $(top_srcdir)/git.mk
