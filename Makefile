CPPFLAGS += -I. $(shell glib-config --cflags) $(shell gtk-config --cflags)
#CFLAGS += -mcpu=i686 -pg -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for regular profiling with gprof
#CFLAGS += -mcpu=i686 -g -pg -ax -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for line by line profiling with gprof
#CFLAGS += -mcpu=i686 -s -O6 -funroll-all-loops -Wall  $(CPPFLAGS) # for maximum speed
CFLAGS += -mcpu=i686 -g -O -fstrict-aliasing -Wall  $(CPPFLAGS) # for debugging
LDFLAGS += $(shell glib-config --libs) $(shell gtk-config --libs) -lm

asm = idct_block_mmx.S
CPPFLAGS += -DUSE_MMX_ASM=1
asm += vlc_x86.S quant_x86.S

sources = playdv.c dct.c weighting.c quant.c vlc.c place.c parse.c bitstream.c ycrcb_to_rgb32.c
objects= $(sources:.c=.o) $(asm:.S=.o)
auxsources=gasmoff.c
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


playdv: $(objects)
	$(CC) -o $@ $(CFLAGS) $(objects) $(LDFLAGS)

dovlc: dovlc.o bitstream.o vlc.o
	$(CC) -o $@ $(CFLAGS) dovlc.o bitstream.o vlc.o $(LDFLAGS)

testvlc: testvlc.o vlc.o vlc_x86.o bitstream.o
	$(CC) -o $@ $(CFLAGS) testvlc.o vlc.o vlc_x86.o bitstream.o $(LDFLAGS)

testbitstream: testbitstream.o bitstream.o
	$(CC) -o $@ $(CFLAGS) testbitstream.o bitstream.o $(LDFLAGS)

vlc_x86.d: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

gasmoff: gasmoff.o bitstream.o

clean:
	rm -f playdv gasmoff asmoff.h *.o *.d 

ifneq ($(MAKECMDGOALS),clean)
-include $(deps)
endif

