CPPFLAGS += -I. $(shell glib-config --cflags) $(shell gtk-config --cflags) $(shell sdl-config --cflags)
#CFLAGS += -mcpu=i686 -pg -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for regular profiling with gprof
#CFLAGS += -mcpu=i686 -g -pg -ax -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for line by line profiling with gprof
#CFLAGS += -mcpu=i686 -s -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for maximum speed
CFLAGS += -mcpu=i686 -g -O -fstrict-aliasing -Wall  $(CPPFLAGS) # for debugging
LDFLAGS += $(shell glib-config --libs) $(shell gtk-config --libs) $(shell sdl-config --libs) -lm

CPPFLAGS += -DUSE_MMX_ASM=1 -DHAVE_XV40x=1 -DHAVE_GTK=1 -DHAVE_SDL=1
asm = vlc_x86.S quant_x86.S idct_block_mmx.S 

gensources=dv.c dct.c idct_248.c weighting.c quant.c vlc.c place.c parse.c bitstream.c YUY2.c YV12.c rgb.c 
genobjects=$(gensources:.c=.o) $(asm:.S=.o)

sources = $(gensources)
objects= $(genobjects)
auxsources=display.c gasmoff.c 
deps=$(sources:.c=.d) $(asm:.S=.d) $(auxsources:.c=.d)

%.d: %.c
	@echo Making $@
	@$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
                           | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
                           [ -s $@ ] || rm -f $@'

%.d: %.S
	@echo Making $@
	@$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
                           | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
                           [ -s $@ ] || rm -f $@'


all: libdv.a playdv

libdv.a: $(objects) 
	$(AR) cru $@ $?
	$(AR) s $@

playdv: playdv.o display.o $(objects)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)


dovlc: dovlc.o bitstream.o vlc.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

testvlc: testvlc.o vlc.o vlc_x86.o bitstream.o
	$(CC) -o $@ $(CFLAGS) $^  $(LDFLAGS)

testbitstream: testbitstream.o bitstream.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

encode: encode.o $(genobjects)
	@echo $(genobjects)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

vlc_x86.d: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

gasmoff: gasmoff.o bitstream.o

clean:
	rm -f libdv.a playdv dovlc testvlc testbitstream encode gasmoff asmoff.h *.o *.d 

ifneq ($(MAKECMDGOALS),clean)
-include $(deps)
endif
