# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/libdv
pkglibdir = $(libdir)/libdv
pkgincludedir = $(includedir)/libdv

top_builddir = .

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux
host_triplet = i686-pc-linux-gnu
AS = @AS@
CC = gcc
DLLTOOL = @DLLTOOL@
GLIB_CFLAGS = -I/usr/lib/glib/include
GLIB_CONFIG = /usr/bin/glib-config
GLIB_LIBS = -L/usr/lib -lglib
GTK_CFLAGS = -I/usr/lib/glib/include -I/usr/X11R6/include
GTK_CONFIG = /usr/bin/gtk-config
GTK_LIBS = -L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
MAINT = 
MAKEINFO = makeinfo
OBJDUMP = @OBJDUMP@
PACKAGE = libdv
RANLIB = ranlib
SDL_CFLAGS = 
SDL_CONFIG = /usr/bin/sdl-config
SDL_LIBS = 
VERSION = 0.1
ac_aux_dir = config

AUX_DIST = $(ac_aux_dir)/config.guess                                    $(ac_aux_dir)/config.sub                                    $(ac_aux_dir)/install-sh                                    $(ac_aux_dir)/ltconfig                                    $(ac_aux_dir)/ltmain.sh                                    $(ac_aux_dir)/missing                                    $(ac_aux_dir)/mkinstalldirs

EXTRA_DIST = bootstrap

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config-h.in                             stamp-h.in $(AUX_DIST)


lib_LTLIBRARIES = libdv.la
bin_PROGRAMS = playdv

BUILT_SOURCES = asmoff.h
EXTRA_PROGRAMS = dovlc testvlc testbitstream encode gasmoff

ASMS = vlc_x86.S quant_x86.S idct_block_mmx.S asmoff.h  mmx.h 
#ASMS = 
gasmoff_SOURCES = gasmoff.c bitstream.c bitstream.h 
#gasmoff_SOURCES = 

pkginclude_HEADERS = dv.h rgb.h display.h
libdv_la_SOURCES = dv.c dct.c idct_248.c weighting.c quant.c vlc.c place.c 	parse.c bitstream.c YUY2.c YV12.c rgb.c 	YUY2.h    bitstream.h  dv.h        parse.h  rgb.h 	YV12.h    dct.h        idct_248.h  place.h  vlc.h 	quant.h  weighting.h $(ASMS)

libdv_la_LDFLAGS = -version-info 0:0:0

playdv_SOURCES = playdv.c display.c display.h
playdv_LDADD = libdv.la

dovlc_SOURCES = dovlc.c bitstream.c vlc.c bitstream.h vlc.h
testvlc_SOURCES = testvlc.c vlc.c parse.c bitstream.c vlc.h bitstream.h
testbitstream_SOURCES = testbitstream.c bitstream.c bitstream.h
encode_SOURCES = encode.c
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
mkinstalldirs = $(SHELL) $(top_srcdir)/config/mkinstalldirs
CONFIG_HEADER = config.h
CONFIG_CLEAN_FILES = 
LTLIBRARIES =  $(lib_LTLIBRARIES)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I.
CPPFLAGS = 
LDFLAGS = 
LIBS = -lm -lXv  -L/usr/lib -lglib -L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm
libdv_la_LIBADD = 
#libdv_la_OBJECTS =  dv.lo dct.lo idct_248.lo \
#weighting.lo quant.lo vlc.lo place.lo parse.lo \
#bitstream.lo YUY2.lo YV12.lo rgb.lo
libdv_la_OBJECTS =  dv.lo dct.lo idct_248.lo weighting.lo \
quant.lo vlc.lo place.lo parse.lo bitstream.lo YUY2.lo \
YV12.lo rgb.lo vlc_x86.lo quant_x86.lo idct_block_mmx.lo
PROGRAMS =  $(bin_PROGRAMS)

dovlc_OBJECTS =  dovlc.o bitstream.o vlc.o
dovlc_LDADD = $(LDADD)
dovlc_DEPENDENCIES = 
dovlc_LDFLAGS = 
testvlc_OBJECTS =  testvlc.o vlc.o parse.o bitstream.o
testvlc_LDADD = $(LDADD)
testvlc_DEPENDENCIES = 
testvlc_LDFLAGS = 
testbitstream_OBJECTS =  testbitstream.o bitstream.o
testbitstream_LDADD = $(LDADD)
testbitstream_DEPENDENCIES = 
testbitstream_LDFLAGS = 
encode_OBJECTS =  encode.o
encode_LDADD = $(LDADD)
encode_DEPENDENCIES = 
encode_LDFLAGS = 
#gasmoff_OBJECTS = 
gasmoff_OBJECTS =  gasmoff.o bitstream.o
gasmoff_LDADD = $(LDADD)
gasmoff_DEPENDENCIES = 
gasmoff_LDFLAGS = 
playdv_OBJECTS =  playdv.o display.o
playdv_DEPENDENCIES =  libdv.la
playdv_LDFLAGS = 
CFLAGS = -g -O2 -I/usr/lib/glib/include -I/usr/lib/glib/include -I/usr/X11R6/include
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
HEADERS =  $(pkginclude_HEADERS)

DIST_COMMON =  README ./stamp-h.in AUTHORS COPYING ChangeLog INSTALL \
Makefile.am Makefile.in NEWS TODO acconfig.h aclocal.m4 config.h.in \
configure configure.in ltconfig


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = gtar
GZIP_ENV = --best
DEP_FILES =  .deps/YUY2.P .deps/YV12.P .deps/bitstream.P .deps/dct.P \
.deps/display.P .deps/dovlc.P .deps/dv.P .deps/encode.P .deps/gasmoff.P \
.deps/idct_248.P .deps/idct_block_mmx.P .deps/parse.P .deps/place.P \
.deps/playdv.P .deps/quant.P .deps/quant_x86.P .deps/rgb.P \
.deps/testbitstream.P .deps/testvlc.P .deps/vlc.P .deps/vlc_x86.P \
.deps/weighting.P
SOURCES = $(libdv_la_SOURCES) $(dovlc_SOURCES) $(testvlc_SOURCES) $(testbitstream_SOURCES) $(encode_SOURCES) $(gasmoff_SOURCES) $(playdv_SOURCES)
OBJECTS = $(libdv_la_OBJECTS) $(dovlc_OBJECTS) $(testvlc_OBJECTS) $(testbitstream_OBJECTS) $(encode_OBJECTS) $(gasmoff_OBJECTS) $(playdv_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .s
$(srcdir)/Makefile.in:  Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

$(ACLOCAL_M4):  configure.in 
	cd $(srcdir) && $(ACLOCAL)

config.status: $(srcdir)/configure $(CONFIG_STATUS_DEPENDENCIES)
	$(SHELL) ./config.status --recheck
$(srcdir)/configure: $(srcdir)/configure.in $(ACLOCAL_M4) $(CONFIGURE_DEPENDENCIES)
	cd $(srcdir) && $(AUTOCONF)

config.h: stamp-h
	@if test ! -f $@; then \
		rm -f stamp-h; \
		$(MAKE) stamp-h; \
	else :; fi
stamp-h: $(srcdir)/config.h.in $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES= CONFIG_HEADERS=config.h \
	     $(SHELL) ./config.status
	@echo timestamp > stamp-h 2> /dev/null
$(srcdir)/config.h.in: $(srcdir)/stamp-h.in
	@if test ! -f $@; then \
		rm -f $(srcdir)/stamp-h.in; \
		$(MAKE) $(srcdir)/stamp-h.in; \
	else :; fi
$(srcdir)/stamp-h.in: $(top_srcdir)/configure.in $(ACLOCAL_M4) acconfig.h
	cd $(top_srcdir) && $(AUTOHEADER)
	@echo timestamp > $(srcdir)/stamp-h.in 2> /dev/null

mostlyclean-hdr:

clean-hdr:

distclean-hdr:
	-rm -f config.h

maintainer-clean-hdr:

mostlyclean-libLTLIBRARIES:

clean-libLTLIBRARIES:
	-test -z "$(lib_LTLIBRARIES)" || rm -f $(lib_LTLIBRARIES)

distclean-libLTLIBRARIES:

maintainer-clean-libLTLIBRARIES:

install-libLTLIBRARIES: $(lib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	@list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(LIBTOOL)  --mode=install $(INSTALL) $$p $(DESTDIR)$(libdir)/$$p"; \
	    $(LIBTOOL)  --mode=install $(INSTALL) $$p $(DESTDIR)$(libdir)/$$p; \
	  else :; fi; \
	done

uninstall-libLTLIBRARIES:
	@$(NORMAL_UNINSTALL)
	list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  $(LIBTOOL)  --mode=uninstall rm -f $(DESTDIR)$(libdir)/$$p; \
	done

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

libdv.la: $(libdv_la_OBJECTS) $(libdv_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libdv_la_LDFLAGS) $(libdv_la_OBJECTS) $(libdv_la_LIBADD) $(LIBS)

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

dovlc: $(dovlc_OBJECTS) $(dovlc_DEPENDENCIES)
	@rm -f dovlc
	$(LINK) $(dovlc_LDFLAGS) $(dovlc_OBJECTS) $(dovlc_LDADD) $(LIBS)

testvlc: $(testvlc_OBJECTS) $(testvlc_DEPENDENCIES)
	@rm -f testvlc
	$(LINK) $(testvlc_LDFLAGS) $(testvlc_OBJECTS) $(testvlc_LDADD) $(LIBS)

testbitstream: $(testbitstream_OBJECTS) $(testbitstream_DEPENDENCIES)
	@rm -f testbitstream
	$(LINK) $(testbitstream_LDFLAGS) $(testbitstream_OBJECTS) $(testbitstream_LDADD) $(LIBS)

encode: $(encode_OBJECTS) $(encode_DEPENDENCIES)
	@rm -f encode
	$(LINK) $(encode_LDFLAGS) $(encode_OBJECTS) $(encode_LDADD) $(LIBS)

gasmoff: $(gasmoff_OBJECTS) $(gasmoff_DEPENDENCIES)
	@rm -f gasmoff
	$(LINK) $(gasmoff_LDFLAGS) $(gasmoff_OBJECTS) $(gasmoff_LDADD) $(LIBS)

playdv: $(playdv_OBJECTS) $(playdv_DEPENDENCIES)
	@rm -f playdv
	$(LINK) $(playdv_LDFLAGS) $(playdv_OBJECTS) $(playdv_LDADD) $(LIBS)

install-pkgincludeHEADERS: $(pkginclude_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkgincludedir)
	@list='$(pkginclude_HEADERS)'; for p in $$list; do \
	  if test -f "$$p"; then d= ; else d="$(srcdir)/"; fi; \
	  echo " $(INSTALL_DATA) $$d$$p $(DESTDIR)$(pkgincludedir)/$$p"; \
	  $(INSTALL_DATA) $$d$$p $(DESTDIR)$(pkgincludedir)/$$p; \
	done

uninstall-pkgincludeHEADERS:
	@$(NORMAL_UNINSTALL)
	list='$(pkginclude_HEADERS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pkgincludedir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES) config.h.in $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)config.h.in$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags config.h.in $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(PACKAGE)-$(VERSION)
top_distdir = $(distdir)

# This target untars the dist file and tries a VPATH configuration.  Then
# it guarantees that the distribution is self-contained by making another
# tarfile.
distcheck: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) zxf $(distdir).tar.gz
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	dc_install_base=`cd $(distdir)/=inst && pwd`; \
	cd $(distdir)/=build \
	  && ../configure --srcdir=.. --prefix=$$dc_install_base \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
	  && $(MAKE) $(AM_MAKEFLAGS) check \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) dist
	-rm -rf $(distdir)
	@banner="$(distdir).tar.gz is ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"
dist: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
dist-all: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
distdir: $(DISTFILES)
	-rm -rf $(distdir)
	mkdir $(distdir)
	-chmod 777 $(distdir)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --foreign Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
	$(MAKE) $(AM_MAKEFLAGS) top_distdir="$(top_distdir)" distdir="$(distdir)" dist-hook

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:
	-rm -rf .deps

maintainer-clean-depend:

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm .deps/$(*F).pp

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-sed -e 's/^\([^:]*\)\.o[ 	]*:/\1.lo \1.o :/' \
	  < .deps/$(*F).pp > .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm -f .deps/$(*F).pp
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
all-recursive-am: config.h
	$(MAKE) $(AM_MAKEFLAGS) all-recursive

install-exec-am: install-libLTLIBRARIES install-binPROGRAMS
install-exec: install-exec-am

install-data-am: install-pkgincludeHEADERS
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-libLTLIBRARIES uninstall-binPROGRAMS \
		uninstall-pkgincludeHEADERS
uninstall: uninstall-am
all-am: Makefile $(LTLIBRARIES) $(PROGRAMS) $(HEADERS) config.h
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(libdir) $(DESTDIR)$(bindir) \
		$(DESTDIR)$(pkgincludedir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	-test -z "$(BUILT_SOURCES)$(MAINTAINERCLEANFILES)" || rm -f $(BUILT_SOURCES) $(MAINTAINERCLEANFILES)
mostlyclean-am:  mostlyclean-hdr mostlyclean-libLTLIBRARIES \
		mostlyclean-compile mostlyclean-libtool \
		mostlyclean-binPROGRAMS mostlyclean-tags \
		mostlyclean-depend mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-hdr clean-libLTLIBRARIES clean-compile clean-libtool \
		clean-binPROGRAMS clean-tags clean-depend clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-hdr distclean-libLTLIBRARIES distclean-compile \
		distclean-libtool distclean-binPROGRAMS distclean-tags \
		distclean-depend distclean-generic clean-am
	-rm -f libtool

distclean: distclean-am
	-rm -f config.status

maintainer-clean-am:  maintainer-clean-hdr \
		maintainer-clean-libLTLIBRARIES \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-binPROGRAMS maintainer-clean-tags \
		maintainer-clean-depend maintainer-clean-generic \
		distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am
	-rm -f config.status

.PHONY: mostlyclean-hdr distclean-hdr clean-hdr maintainer-clean-hdr \
mostlyclean-libLTLIBRARIES distclean-libLTLIBRARIES \
clean-libLTLIBRARIES maintainer-clean-libLTLIBRARIES \
uninstall-libLTLIBRARIES install-libLTLIBRARIES mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS uninstall-pkgincludeHEADERS \
install-pkgincludeHEADERS tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir mostlyclean-depend \
distclean-depend clean-depend maintainer-clean-depend info-am info \
dvi-am dvi check check-am installcheck-am installcheck all-recursive-am \
install-exec-am install-exec install-data-am install-data install-am \
install uninstall-am uninstall all-redirect all-am all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
		cp $$file $(distdir)/$$file; \
	done

# Automake doesn't do dependency tracking for asm
quant_x86.lo vlc_x86.lo: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
