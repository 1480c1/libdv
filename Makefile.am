SUBDIRS                   = libdv playdv encodedv

AUX_DIST                  = $(ac_aux_dir)/config.guess \
                                   $(ac_aux_dir)/config.sub \
                                   $(ac_aux_dir)/install-sh \
                                   $(ac_aux_dir)/ltconfig \
                                   $(ac_aux_dir)/ltmain.sh \
                                   $(ac_aux_dir)/missing \
                                   $(ac_aux_dir)/mkinstalldirs 

AUX_DIST_EXTRA		  = $(ac_aux_dir)/ac_c_bigendian_cross.m4 \
                                   $(ac_aux_dir)/README

EXTRA_DIST                = bootstrap COPYRIGHT README.encoder README.dvconnect \
                            @PACKAGE@.spec @PACKAGE@.pc

MAINTAINERCLEANFILES      = Makefile.in aclocal.m4 configure config.h.in \
                            stamp-h.in $(AUX_DIST)

CLEANFILES                = 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdv.pc

rpm: dist
	rpm -ta @PACKAGE@-@VERSION@.tar.gz

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
		cp $$file $(distdir)/$$file; \
	done

maintainer-clean-local:
	rm -f libdv*.tar.gz
