AUX_DIST                  = $(ac_aux_dir)/config.guess \
                                   $(ac_aux_dir)/config.sub \
                                   $(ac_aux_dir)/install-sh \
                                   $(ac_aux_dir)/ltconfig \
                                   $(ac_aux_dir)/ltmain.sh \
                                   $(ac_aux_dir)/missing \
                                   $(ac_aux_dir)/mkinstalldirs
EXTRA_DIST                = bootstrap

MAINTAINERCLEANFILES      = Makefile.in aclocal.m4 configure config.h.in \
                            stamp-h.in $(AUX_DIST)

CLEANFILES                = asmoff.h $(EXTRA_PROGRAMS)

lib_LTLIBRARIES= libdv.la
bin_PROGRAMS= playdv

BUILT_SOURCES = asmoff.h
EXTRA_PROGRAMS= dovlc testvlc testbitstream encode gasmoff

#
# If HOST_X86 is set, we build all the x86 asm stuff..
#
if HOST_X86

ASMS= vlc_x86.S quant_x86.S idct_block_mmx.S dct_block_mmx.S rgbtoyuv.S asmoff.h  mmx.h 
gasmoff_SOURCES= gasmoff.c bitstream.c bitstream.h 

else # HOST_X86

ASMS=
gasmoff_SOURCES=

endif # HOST_X86

pkginclude_HEADERS= dv.h dv_types.h bitstream.h display.h bitstream.h oss.h
libdv_la_SOURCES= dv.c dct.c idct_248.c weighting.c quant.c vlc.c place.c \
	parse.c bitstream.c YUY2.c YV12.c rgb.c audio.c oss.c \
	YUY2.h    bitstream.h  dv.h        parse.h  rgb.h \
	YV12.h    dct.h        idct_248.h  place.h  vlc.h \
	quant.h  weighting.h audio.h rgb.h audio.h $(ASMS)
libdv_la_LDFLAGS = -version-info 0:0:0

playdv_SOURCES= playdv.c display.c display.h
playdv_LDADD= libdv.la

dovlc_SOURCES= dovlc.c bitstream.c vlc.c bitstream.h vlc.h
dovlc_LDADD= libdv.la

testvlc_SOURCES= testvlc.c vlc.c parse.c bitstream.c vlc.h bitstream.h
testvlc_LDADD=libdv.la

testbitstream_SOURCES= testbitstream.c bitstream.c bitstream.h

encode_SOURCES= encode.c 
encode_LDADD= libdv.la

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
		cp $$file $(distdir)/$$file; \
	done

if HOST_X86

# Automake doesn't do dependency tracking for asm
quant_x86.lo vlc_x86.lo: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

endif