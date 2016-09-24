SUBDIRS = . libdunfell/tests libdunfell-ui/tests libdunfell/docs libdunfell-ui/docs

ACLOCAL_AMFLAGS = -I m4

AM_DISTCHECK_CONFIGURE_FLAGS =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST =
bin_PROGRAMS =
bin_SCRIPTS =
man8_MANS =
VAPIGEN_VAPIS =
lib_LTLIBRARIES =
BUILT_SOURCES =

# libdunfell library
lib_LTLIBRARIES += libdunfell/libdunfell-@DFL_API_VERSION@.la

dflincludedir = $(includedir)/libdunfell-@DFL_API_VERSION@
dfl_headers = \
	libdunfell/event.h \
	libdunfell/event-sequence.h \
	libdunfell/main-context.h \
	libdunfell/model.h \
	libdunfell/parser.h \
	libdunfell/source.h \
	libdunfell/task.h \
	libdunfell/thread.h \
	libdunfell/time-sequence.h \
	libdunfell/types.h \
	libdunfell/version.h \
	$(NULL)

# The following headers are private, and shouldn't be installed:
dfl_private_headers = \
	$(NULL)
nobase_dflinclude_HEADERS = \
	$(dfl_main_header) \
	$(dfl_headers) \
	$(NULL)

dfl_sources = \
	libdunfell/event.c \
	libdunfell/event-sequence.c \
	libdunfell/main-context.c \
	libdunfell/model.c \
	libdunfell/parser.c \
	libdunfell/source.c \
	libdunfell/task.c \
	libdunfell/thread.c \
	libdunfell/time-sequence.c \
	$(NULL)

dfl_main_header = libdunfell/dunfell.h
dfl_public_headers = $(nobase_dflinclude_HEADERS)

libdunfell_libdunfell_@DFL_API_VERSION@_la_SOURCES = \
	$(dfl_private_headers) \
	$(dfl_sources) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
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
	$(AM_LIBADD) \
	$(NULL)

libdunfell_libdunfell_@DFL_API_VERSION@_la_LDFLAGS = \
	-version-info $(DFL_LT_VERSION) \
	-export-symbols-regex "^dfl_" \
	-no-undefined \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

# Check if dunfell.h includes all the public headers
check-local: check-headers
check-headers:
	@any_missing=0; for header_file in $(dfl_public_headers); do \
		if test "x$$header_file" != "x$(dfl_main_header)"; then \
			if ! grep "#include <$$header_file>" $(top_srcdir)/$(dfl_main_header) >/dev/null; then \
				echo "$(dfl_main_header) doesn't appear to include \"$$header_file\""; \
				any_missing=1; \
			fi; \
		fi; \
	done; exit "$$any_missing"

# libdunfell-ui library
lib_LTLIBRARIES += libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la

dwlincludedir = $(includedir)/libdunfell-ui-@DWL_API_VERSION@
dwl_headers = \
	libdunfell-ui/source-model.h \
	libdunfell-ui/statistics-pane.h \
	libdunfell-ui/task-model.h \
	libdunfell-ui/timeline.h \
	$(NULL)
nodist_dwl_headers = \
	libdunfell-ui/enums.h \
	libdunfell-ui/version.h \
	$(NULL)

# The following headers are private, and shouldn't be installed:
dwl_private_headers = \
	$(NULL)
nobase_dwlinclude_HEADERS = \
	$(dwl_main_header) \
	$(dwl_headers) \
	$(NULL)
nodist_dwlinclude_HEADERS = \
	$(nodist_dwl_headers) \
	$(NULL)

dwl_sources = \
	libdunfell-ui/source-model.c \
	libdunfell-ui/statistics-pane.c \
	libdunfell-ui/task-model.c \
	libdunfell-ui/timeline.c \
	$(NULL)
nodist_dwl_sources = \
	libdunfell-ui/enums.c \
	libdunfell-ui/resources.c \
	$(NULL)

dwl_main_header = libdunfell-ui/dunfell-ui.h
dwl_public_headers = $(nobase_dwlinclude_HEADERS)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_SOURCES = \
	$(dwl_private_headers) \
	$(dwl_sources) \
	$(NULL)
nodist_libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_SOURCES = \
	$(nodist_dwl_sources) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
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
	$(top_builddir)/libdunfell/libdunfell-@DFL_API_VERSION@.la \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(AM_LIBADD) \
	$(NULL)

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_LDFLAGS = \
	-version-info $(DWL_LT_VERSION) \
	-export-symbols-regex "^dwl_" \
	-no-undefined \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

dwl_enum_header_file = libdunfell-ui/enums.h
dwl_enum_source_file = libdunfell-ui/enums.c
dwl_enum_prerequisites = $(dwl_public_headers)

$(dwl_enum_header_file): $(dwl_enum_header_file:%=%.template) $(dwl_enum_prerequisites)
	$(AM_V_GEN)$(GLIB_MKENUMS) --template $< \
		$(patsubst %,$(srcdir)/%,$(dwl_enum_prerequisites)) \
		> $@.tmp && mv $@.tmp $@

$(dwl_enum_source_file): $(dwl_enum_source_file:%=%.template) $(dwl_enum_prerequisites)
	$(AM_V_GEN)$(GLIB_MKENUMS) --template $< \
		$(patsubst %,$(srcdir)/%,$(dwl_enum_prerequisites)) \
		> $@.tmp && mv $@.tmp $@

EXTRA_DIST += \
	$(dwl_enum_header_file:%=%.template) \
	$(dwl_enum_source_file:%=%.template) \
	$(NULL)
BUILT_SOURCES += \
	$(dwl_enum_header_file) \
	$(dwl_enum_source_file) \
	$(NULL)
CLEANFILES += \
	$(dwl_enum_header_file) \
	$(dwl_enum_source_file) \
	$(NULL)

libdunfell_ui_org_gnome_libdunfell_ui_gresource_xml_deps = \
	$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(top_srcdir)/libdunfell-ui $(top_srcdir)/libdunfell-ui/org.gnome.libdunfell-ui.gresource.xml)
libdunfell-ui/resources.c: $(libdunfell_ui_org_gnome_libdunfell_ui_gresource_xml_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/libdunfell-ui --generate-source $(top_srcdir)/libdunfell-ui/org.gnome.libdunfell-ui.gresource.xml

BUILT_SOURCES += libdunfell-ui/resources.c
CLEANFILES += libdunfell-ui/resources.c
EXTRA_DIST += \
	libdunfell-ui/org.gnome.libdunfell-ui.gresource.xml \
	$(libdunfell_ui_org_gnome_libdunfell_ui_gresource_xml_deps) \
	$(NULL)

# Check if dunfell-ui.h includes all the public headers
check-local: check-dwl-headers
check-dwl-headers:
	@any_missing=0; for header_file in $(dwl_public_headers); do \
		if test "x$$header_file" != "x$(dwl_main_header)"; then \
			if ! grep "#include <$$header_file>" $(top_srcdir)/$(dwl_main_header) >/dev/null; then \
				echo "$(dwl_main_header) doesn't appear to include \"$$header_file\""; \
				any_missing=1; \
			fi; \
		fi; \
	done; exit "$$any_missing"

.PHONY: check-dwl-headers

# dunfell-record program
dfldatadir = $(datadir)/libdunfell-@DFL_API_VERSION@
dist_dfldata_DATA = record/dunfell-record.stp

EXTRA_DIST += record/dunfell-record.in
bin_SCRIPTS += record/dunfell-record
CLEANFILES += record/dunfell-record

record/dunfell-record: $(srcdir)/record/dunfell-record.in
	$(AM_V_GEN)$(MKDIR_P) record && \
	sed -e "s,[@]datadir[@],$(datadir),g;s,[@]DFL_API_VERSION[@],@DFL_API_VERSION@,g" $< > $@ && chmod +x $@ || rm $@

# Viewer application
bin_PROGRAMS += viewer/dunfell-viewer

viewer_dunfell_viewer_SOURCES = \
	viewer/application.c \
	viewer/application.h \
	viewer/viewer-window.c \
	viewer/viewer-window.h \
	viewer/main.c \
	$(NULL)
nodist_viewer_dunfell_viewer_SOURCES = \
	viewer/resources.c \
	$(NULL)
viewer_dunfell_viewer_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DG_LOG_DOMAIN=\"dunfell-viewer\" \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS) \
	$(NULL)
viewer_dunfell_viewer_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)
viewer_dunfell_viewer_LDADD = \
	$(top_builddir)/libdunfell/libdunfell-@DFL_API_VERSION@.la \
	$(top_builddir)/libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(AM_LDADD) \
	$(NULL)
viewer_dunfell_viewer_LDFLAGS = \
	-no-undefined \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

viewer_org_gnome_Dunfell_Viewer_gresource_xml_deps = \
	$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(top_srcdir)/viewer $(top_srcdir)/viewer/org.gnome.Dunfell.Viewer.gresource.xml)
viewer/resources.c: $(viewer_org_gnome_Dunfell_Viewer_gresource_xml_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/viewer --generate-source $(top_srcdir)/viewer/org.gnome.Dunfell.Viewer.gresource.xml

BUILT_SOURCES += viewer/resources.c
CLEANFILES += viewer/resources.c
EXTRA_DIST += \
	viewer/org.gnome.Dunfell.Viewer.gresource.xml \
	$(viewer_org_gnome_Dunfell_Viewer_gresource_xml_deps) \
	$(NULL)

desktopdir = $(datadir)/applications
desktop_DATA = viewer/dunfell-viewer.desktop

%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -o $@-t \
		-d $(top_srcdir)/po && \
	mv $@-t $@

EXTRA_DIST += $(desktop_DATA:%=%.in)
CLEANFILES += $(desktop_DATA)

# Introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS =
INTROSPECTION_COMPILER_ARGS =
AM_DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
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
libdunfell_Dunfell_@DFL_API_VERSION@_gir_EXPORT_PACKAGES = libdunfell-@DFL_API_VERSION@
libdunfell_Dunfell_@DFL_API_VERSION@_gir_SCANNERFLAGS = \
	--nsversion=@DFL_API_VERSION@ \
	--identifier-prefix=Dfl \
	--symbol-prefix=dfl \
	--c-include="libdunfell/dunfell.h" \
	$(WARN_SCANNERFLAGS) \
	$(NULL)

INTROSPECTION_GIRS += libdunfell/Dunfell-@DFL_API_VERSION@.gir

libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir: libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la libdunfell/Dunfell-@DFL_API_VERSION@.gir
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_INCLUDES = GObject-2.0 Gio-2.0 Gtk-3.0
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_la_CPPFLAGS) \
	$(NULL)
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_LIBS = libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.la
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_FILES = $(dwl_sources) $(nodist_dwl_sources) $(dwl_headers) $(nodist_dwl_headers)
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_NAMESPACE = DunfellUi
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_EXPORT_PACKAGES = libdunfell-ui-@DWL_API_VERSION@
libdunfell_ui_DunfellUi_@DWL_API_VERSION@_gir_SCANNERFLAGS = \
	--nsversion=@DWL_API_VERSION@ \
	--identifier-prefix=Dwl \
	--symbol-prefix=dwl \
	--c-include="libdunfell-ui/dunfell-ui.h" \
	--include-uninstalled=$(builddir)/libdunfell/Dunfell-@DFL_API_VERSION@.gir \
	$(WARN_SCANNERFLAGS) \
	$(NULL)

INTROSPECTION_GIRS += libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir

INTROSPECTION_COMPILER_ARGS += --includedir $(builddir)/libdunfell

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

libdunfell_libdunfell_@DFL_API_VERSION@_vapi_DEPS = gio-2.0
libdunfell_libdunfell_@DFL_API_VERSION@_vapi_METADATADIRS = $(srcdir)/libdunfell
libdunfell_libdunfell_@DFL_API_VERSION@_vapi_FILES = libdunfell/Dunfell-@DFL_API_VERSION@.gir

libdunfell/libdunfell-@DFL_API_VERSION@.deps:
	$(AM_V_GEN) for pkg in $(libdunfell_libdunfell_@DFL_API_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

# Note: The VAPI file name has to match the pkg-config file name.
libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.vapi: libdunfell-ui/DunfellUi-@DWL_API_VERSION@.gir

VAPIGEN_VAPIS += libdunfell-ui/libdunfell-ui-@DWL_API_VERSION@.vapi

libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_DEPS = gio-2.0 gtk+-3.0
libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_METADATADIRS = $(srcdir)/libdunfell-ui
libdunfell_ui_libdunfell_ui_@DWL_API_VERSION@_vapi_GIRDIRS = $(builddir)/libdunfell
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
	m4/ \
	ABOUT-NLS \
	config.rpath \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	$(NULL)

AM_DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc --disable-vala

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
	libdunfell/version.h.in \
	libdunfell-ui/libdunfell-ui.pc.in \
	libdunfell-ui/version.h.in \
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
