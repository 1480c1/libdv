AUX_DIST                  = $(ac_aux_dir)/config.guess \
                                   $(ac_aux_dir)/config.sub \
                                   $(ac_aux_dir)/install-sh \
                                   $(ac_aux_dir)/ltconfig \
                                   $(ac_aux_dir)/ltmain.sh \
                                   $(ac_aux_dir)/missing \
                                   $(ac_aux_dir)/mkinstalldirs 
AUX_DIST_EXTRA		  = $(ac_aux_dir)/ac_c_bigendian_cross.m4 \
                                   $(ac_aux_dir)/README
 
EXTRA_DIST                = bootstrap COPYRIGHT @PACKAGE@.spec

MAINTAINERCLEANFILES      = Makefile.in aclocal.m4 configure config.h.in \
                            stamp-h.in $(AUX_DIST)

CLEANFILES                = asmoff.h 

lib_LTLIBRARIES= libdv.la
bin_PROGRAMS= playdv encode

if HOST_X86
GASMOFF=gasmoff
else # HOST_X86
GASMOFF=
endif # HOST_X86

noinst_PROGRAMS= dovlc testvlc testbitstream \
	insert_audio fix_headers scan_packet_headers steal_header ppmqscale $(GASMOFF)

#
# If HOST_X86 is set, we build all the x86 asm stuff..
#
if HOST_X86

BUILT_SOURCES = asmoff.h

libdv_la_ASMS= vlc_x86.S quant_x86.S idct_block_mmx.S dct_block_mmx.S rgbtoyuv.S encode_x86.S transpose_x86.S
libdv_la_ASM_HS=asmoff.h  mmx.h 


gasmoff_SOURCES= gasmoff.c bitstream.c bitstream.h 

else # HOST_X86

ASMS=
gasmoff_SOURCES=

endif # HOST_X86

pkginclude_HEADERS=  dv_types.h dv.h display.h oss.h

noinst_HEADERS=  YUY2.h      bitstream.h parse.h  rgb.h \
	YV12.h   dct.h       idct_248.h  place.h  vlc.h \
	quant.h  weighting.h audio.h     rgb.h    audio.h \
        util.h   $(libdv_la_ASM_HS)

libdv_la_SOURCES= dv.c dct.c idct_248.c weighting.c quant.c vlc.c place.c \
	parse.c bitstream.c YUY2.c YV12.c rgb.c audio.c oss.c util.c \
	$(libdv_la_ASMS)

libdv_la_LDFLAGS = -version-info 1:0:0

playdv_SOURCES= playdv.c display.c display.h
playdv_LDADD= libdv.la

dovlc_SOURCES= dovlc.c 
dovlc_LDADD= libdv.la

testvlc_SOURCES= testvlc.c 
testvlc_LDADD=libdv.la

testbitstream_SOURCES= testbitstream.c bitstream.c bitstream.h

encode_SOURCES= encode.c headers.c
encode_LDADD= libdv.la

insert_audio_SOURCES = insert_audio.c headers.c

fix_headers_SOURCES = fix_headers.c headers.c

scan_packet_headers_SOURCES = scan_packet_headers.c

steal_header_SOURCES = steal_header.c

ppmqscale_SOURCES = ppmqscale.c

rpm: dist
	rpm -ta @PACKAGE@-@VERSION@.tar.gz

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
