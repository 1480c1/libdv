man_MANS                  = playdv.1 

EXTRA_DIST                = $(man_MANS)

MAINTAINERCLEANFILES      = Makefile.in $(AUX_DIST)

CLEANFILES                = 

LIBTOOL = $(SHELL) $(top_builddir)/libtool --silent

bin_PROGRAMS= playdv

noinst_HEADERS=  display.h oss.h

playdv_SOURCES= playdv.c display.c display.h oss.c
playdv_LDADD= ../libdv/libdv.la
