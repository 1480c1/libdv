/* 
 *  insert_audio.c
 *
 *     Copyright (C) Peter Schlaile - January 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#include <dv_types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "headers.h"

struct audio_info {
	int channels;
	int frequency;
	int bitspersample;
	int bytespersecond;
	int bytealignment;
};

unsigned long read_long(FILE* in_wav)
{
	unsigned char buf[4];

	if (fread(buf, 1, 4, in_wav) != 4) {
		fprintf(stderr, "WAV: Short read!\n");
		exit(-1);
	}

	return buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
}

unsigned long read_short(FILE* in_wav)
{
	unsigned char buf[2];
	if (fread(buf, 1, 2, in_wav) != 2) {
		fprintf(stderr, "WAV: Short read!\n");
		exit(-1);
	}

	return buf[0] + (buf[1] << 8);
}

void read_header(FILE* in_wav, char* header)
{
	unsigned char buf[4];

	if (fread(buf, 1, 4, in_wav) != 4) {
		fprintf(stderr, "WAV: Short read!\n");
		exit(-1);
	}

	if (memcmp(buf, header, 4) != 0) {
		fprintf(stderr, "WAV: No %s header!\n", header);
		exit(-1);
	}
}

struct audio_info parse_wave_header(FILE* in_wav)
{
	struct audio_info res;
	unsigned char fmt_header_junk[1024];
	int header_len;

	read_header(in_wav, "RIFF");
	read_long(in_wav); /* ignore length, this is important,
			      since stream generated wavs do not have a
			      valid length! */

	read_header(in_wav, "WAVE");
	read_header(in_wav, "fmt ");
	header_len = read_long(in_wav);
	if (header_len > 1024) {
		fprintf(stderr, "WAV: Header too large!\n");
		exit(-1);
	}
	
	read_short(in_wav); /* format tag */
	res.channels = read_short(in_wav);
	res.frequency = read_long(in_wav);
	res.bytespersecond = read_long(in_wav); /* bytes per second */
	res.bytealignment = read_short(in_wav); /* byte alignment */
	res.bitspersample = read_short(in_wav);
	if (header_len - 16) {
		fread(fmt_header_junk, 1, header_len - 16, in_wav);
	}
	read_header(in_wav, "data");
	read_long(in_wav); /* ignore length, this is important,
			      since stream generated wavs do not have a
			      valid length! */

	switch (res.frequency) {
	case 48000:
	case 44100:
		if (res.channels != 2) {
			fprintf(stderr, 
				"WAV: Unsupported channel count (%d) for "
				"frequency %d!\n", res.channels,
				res.frequency);
			exit(-1);
		}
		break;
	case 32000:
		if (res.channels != 4 && res.channels != 2) {
			fprintf(stderr, 
				"WAV: Unsupported channel count (%d) for "
				"frequency %d!\n", res.channels,
				res.frequency);
			exit(-1);
		}
		break;
	default:
		fprintf(stderr, "WAV: Unsupported frequency: %d\n",
			res.frequency);
		exit(-1);
	}
	if (res.bitspersample != 16) {
		fprintf(stderr, "WAV: Unsupported bitspersample: %d Only 16 "
			"bits are supported right now. (FIXME: just look in "
			"audio.c and copy the code if you "
			"really need this!)\n", res.bitspersample);
		exit(-1);
	}

	fprintf(stderr, "Opening WAV file with:\n"
		"Channels: %d\n"
		"Frequency: %d\n"
		"Bytes per second: %d\n"
		"Byte alignment: %d\n"
		"Bits per sample: %d\n",
		res.channels, res.frequency, res.bytespersecond,
		res.bytealignment, res.bitspersample);

	return res;
}

void generate_empty_frame(unsigned char* frame_buf, int isPAL)
{
	static time_t t = 0;
	static int frame_count = -1;
	if (!t) {
		t = time(NULL);
	}
	if (frame_count == -1) {
		frame_count = isPAL ? 25 : 30;
	}
	if (!--frame_count) {
		frame_count = isPAL ? 25 : 30;
		t++;
	}
	memset(frame_buf, 0, isPAL ? 144000 : 120000);
	write_info_blocks(frame_buf, 0, isPAL, &t);
}

int read_frame(FILE* in_vid, unsigned char* frame_buf, int * isPAL)
{
	if (fread(frame_buf, 1, 120000, in_vid) != 120000) {
		generate_empty_frame(frame_buf, *isPAL);
		return 0;
	}

	*isPAL = (frame_buf[3] & 0x80);

	if (*isPAL) {
		if (fread(frame_buf + 120000, 1, 144000 - 120000, in_vid) !=
		    144000 - 120000) {
			generate_empty_frame(frame_buf, *isPAL);
			return 0;
		}
	}
	return 1;
}

static int dv_audio_unshuffle_60[5][9] = {
  { 0, 15, 30, 10, 25, 40,  5, 20, 35 },
  { 3, 18, 33, 13, 28, 43,  8, 23, 38 },
  { 6, 21, 36,  1, 16, 31, 11, 26, 41 },
  { 9, 24, 39,  4, 19, 34, 14, 29, 44 },
  {12, 27, 42,  7, 22, 37,  2, 17, 32 },
};

static int dv_audio_unshuffle_50[6][9] = {
  {  0, 18, 36, 13, 31, 49,  8, 26, 44 },
  {  3, 21, 39, 16, 34, 52, 11, 29, 47 },
  {  6, 24, 42,  1, 19, 37, 14, 32, 50 }, 
  {  9, 27, 45,  4, 22, 40, 17, 35, 53 }, 
  { 12, 30, 48,  7, 24, 43,  2, 20, 38 },
  { 15, 33, 51, 10, 28, 46,  5, 23, 41 },
};

void put_16_bit(unsigned char * target, unsigned char* wave_buf,
		struct audio_info * audio, int dif_seg, int isPAL,
		int channel)
{
	int bp;
	int audio_dif;
	unsigned char* p = target;

	if (isPAL) {
		for (audio_dif = 0; audio_dif < 9; audio_dif++) {
			for (bp = 8; bp < 80; bp += 2) {
				int i = dv_audio_unshuffle_50[dif_seg]
					[audio_dif] + (bp - 8)/2 * 54;
				p[bp] = wave_buf[
					audio->bytealignment * i + 1
					+ 2*channel];
				p[bp + 1] = wave_buf[
					audio->bytealignment * i
					+ 2*channel];
				if (p[bp] == 0x80 && p[bp+1] == 0x00) {
					p[bp+1] = 0x01;
				}
			}
			p += 16 * 80;
		}
	} else {
		for (audio_dif = 0; audio_dif < 9; audio_dif++) {
			for (bp = 8; bp < 80; bp += 2) {
				int i = dv_audio_unshuffle_60[dif_seg]
					[audio_dif] + (bp - 8)/2 * 45;
				p[bp] = wave_buf[
					audio->bytealignment * i + 1
					+ 2*channel];
				p[bp + 1] = wave_buf[
					audio->bytealignment * i
					+ 2*channel];
				if (p[bp] == 0x80 && p[bp+1] == 0x00) {
					p[bp+1] = 0x01;
				}
			}
			p += 16 * 80;
		}
	}
}


#if 0
pond:

 0  1  2  3  4
--------------
50 d3 00 c0 c8 
51 3f cf a0 ff 
52 56 c3 e4 00
53 ff c6 87 d4

50 d4 01 c0 c8
51 3f cf a0 ff
52 56 c3 e4 00
53 ff c6 87 d4

eule:
50 d0 30 e0 d1
51 33 cf a0 ff
52 ff c9 e1 01
53 ff c1 99 d6

50 d0 3f e0 d1
51 3f ff a0 ff
52 ff c9 e1 01
53 ff c1 99 d6

Programm:
50 56 30 a0 08
51 33 cf a0 9c
52 ff 14 00 01
53 ff 33 05 14
#endif

void insert_audio(unsigned char * frame_buf, unsigned char* wave_buf, 
		  struct audio_info * audio, int isPAL)
{
	int dif_seg;
	int dif_seg_max = isPAL ? 12 : 10;
	int samplesperframe = audio->frequency / (isPAL ? 25 : 30);
	
	int bits_per_sample = 16;

	unsigned char head_50[5];
	unsigned char head_51[5];
	unsigned char head_52[5];
	unsigned char head_53[5];

	head_50[0] = 0x50;

	if (isPAL) {
		head_50[3] = /* stype = */ 0 | (/* isPAL */ 1 << 5)
			| (/* ml */ 1 << 6) | (/* res */ 1 << 7);
		switch(audio->frequency) {
		case 32000:
			if (audio->channels == 2) {
				head_50[1] = (samplesperframe - 1264) 
					| (1 << 6) | (/* unlocked = */ 1 << 7);
				head_50[2] = /* audio_mode= */ 0 
					| (/* pa = */ 0 << 4) 
					| (/* chn = */ 0 << 5)
					| (/* sm = */ 0 << 7);
				head_50[4] = /* 12 Bits */0 
					| (/* 32000 kHz */ 2 << 3)
					| (/* tc = */ 0 << 6) 
					| (/* ef = */ 0 << 7);
			} else {
				head_50[1] = (samplesperframe - 1264) 
					| (1 << 6) | (/* unlocked = */ 1 << 7);
				head_50[2] = /* audio_mode= */ 0 
					| (/* pa = */ 1 << 4) 
					| (/* chn = */ 1 << 5)
					| (/* sm = */ 0 << 7);
				head_50[4] = /* 12 Bits */1 
					| (/* 32000 kHz */ 2 << 3)
					| (/* tc = */ 0 << 6) 
					| (/* ef = */ 0 << 7);
				bits_per_sample = 12;
			}
			break;
		case 44100:
			head_50[1] = (samplesperframe - 1742) | (1 << 6)
				| (/* unlocked = */ 1 << 7);
			head_50[2] = /* audio_mode= */ 0 | (/* pa = */ 0 << 4) 
				| (/* chn = */ 0 << 5)
				| (/* sm = */ 0 << 7);
			head_50[4] = /* 16 Bits */0 | (/* 44100 kHz */ 1 << 3)
				| (/* tc = */ 0 << 6) | (/* ef = */ 0 << 7);
			break;
		case 48000:
			head_50[1] = (samplesperframe - 1896) | (1 << 6)
				| (/* unlocked = */ 1 << 7);
			head_50[2] = /* audio_mode= */ 0 | (/* pa = */ 0 << 4) 
				| (/* chn = */ 0 << 5);
			head_50[4] = /* 16 Bits */0 | (/* 48000 kHz */ 0 << 3)
				| (/* tc = */ 0 << 6) | (/* ef = */ 0 << 7);
			break;
		default:
			fprintf(stderr, "Impossible frequency??\n");
			exit(-1);
		}
	} else {
		head_50[3] = /* stype = */ 0 | (/* isPAL */ 0 << 5)
			| (/* ml */ 1 << 6) | (/* res */ 1 << 7);
		switch(audio->frequency) {
		case 32000:
			if (audio->channels == 2) {
				head_50[1] = (samplesperframe - 1053) 
					| (1 << 6) | (/* unlocked = */ 1 << 7);
				head_50[2] = /* audio_mode= */ 0 
					| (/* pa = */ 0 << 4) 
					| (/* chn = */ 0 << 5)
					| (/* sm = */ 0 << 7);
				head_50[4] = /* 12 Bits */0 
					| (/* 32000 kHz */ 2 << 3)
					| (/* tc = */ 0 << 6) 
					| (/* ef = */ 0 << 7);
			} else {
				head_50[1] = (samplesperframe - 1053) 
					| (1 << 6) | (/* unlocked = */ 1 << 7);
				head_50[2] = /* audio_mode= */ 0 
					| (/* pa = */ 1 << 4) 
					| (/* chn = */ 1 << 5)
					| (/* sm = */ 0 << 7);
				head_50[4] = /* 12 Bits */1 
					| (/* 32000 kHz */ 2 << 3)
					| (/* tc = */ 0 << 6) 
					| (/* ef = */ 0 << 7);
				bits_per_sample = 12;
			}
			break;
		case 44100:
			head_50[1] = (samplesperframe - 1452) | (1 << 6)
				| (/* unlocked = */ 1 << 7);
			head_50[2] = /* audio_mode= */ 0 | (/* pa = */ 0 << 4) 
				| (/* chn = */ 0 << 5)
				| (/* sm = */ 0 << 7);
			head_50[4] = /* 16 Bits */0 | (/* 44100 kHz */ 1 << 3)
				| (/* tc = */ 0 << 6) | (/* ef = */ 0 << 7);
			break;
		case 48000:
			head_50[1] = (samplesperframe - 1580) | (1 << 6)
				| (/* unlocked = */ 1 << 7);
			head_50[2] = /* audio_mode= */ 0 | (/* pa = */ 0 << 4) 
				| (/* chn = */ 0 << 5);
			head_50[4] = /* 16 Bits */0 | (/* 48000 kHz */ 0 << 3)
				| (/* tc = */ 0 << 6) | (/* ef = */ 0 << 7);
			break;
		default:
			fprintf(stderr, "Impossible frequency??\n");
			exit(-1);
		}
	}

	head_51[0] = 0x51; /* FIXME: What's this? */ 
	head_51[1] = 0x33;
	head_51[2] = 0xcf;
	head_51[3] = 0xa0;

	head_52[0] = 0x52;
	head_52[1] = frame_buf[5 * 80 + 48 + 2 * 5 + 1]; /* steal video */
	head_52[2] = frame_buf[5 * 80 + 48 + 2 * 5 + 2]; /* timestamp */
	head_52[3] = frame_buf[5 * 80 + 48 + 2 * 5 + 3]; /* this gets us an */
	head_52[4] = frame_buf[5 * 80 + 48 + 2 * 5 + 4]; /* off by one error!*/
	                                                   
	head_53[0] = 0x53; 
	head_53[1] = frame_buf[5 * 80 + 48 + 3 * 5 + 1];
	head_53[2] = frame_buf[5 * 80 + 48 + 3 * 5 + 2];
	head_53[3] = frame_buf[5 * 80 + 48 + 3 * 5 + 3];
	head_53[4] = frame_buf[5 * 80 + 48 + 3 * 5 + 4];

	for (dif_seg = 0; dif_seg < dif_seg_max; dif_seg++) {
		int audio_dif;
		unsigned char* target= frame_buf + dif_seg * 150 * 80 + 6 * 80;
		int channel;
		int ds;

		unsigned char* p = target + 3;
		for (audio_dif = 0; audio_dif < 9; audio_dif++) {
			memset(p, 0xff, 5);
			p += 16 * 80;
		}

		if ((dif_seg & 1) == 0) {
			p = target + 3 * 16 * 80 + 3;
		} else {
			p = target + 3;
		}

		/* Timestamp it! */
		memcpy(p + 0*16*80, head_50, 5);
		memcpy(p + 1*16*80, head_51, 5);
		memcpy(p + 2*16*80, head_52, 5);
		memcpy(p + 3*16*80, head_53, 5);

		if (dif_seg >= dif_seg_max/2) {
			p[2] |= 1;
		}

		switch(bits_per_sample) {
		case 12:
			fprintf(stderr, "Unsupported bits: 12\n FIXME!\n");
			exit(-1);
		case 16:
			ds = dif_seg;
			if(ds < dif_seg_max/2) {
				channel = 0;
			} else {
				channel = 1;
				ds -= dif_seg_max / 2;
			} 
			put_16_bit(target, wave_buf,
				   audio, ds, isPAL, channel);
			break;
		}
	}
	
	
}

int main(int argc, const char** argv)
{
	FILE* in_wav;
	FILE* in_vid;
	FILE* out_vid;

	unsigned char wave_buf[1920 * 2 * 2]; /* max 48000.0 kHz PAL */
	unsigned char frame_buf[144000];
	struct audio_info audio;
	int isPAL = 1;
	int bytesperframe = 0;
	int gotframe = 0;
	int have_pipes = 0;

	if (argc != 3 && argc != 2) {
		fprintf(stderr, "Usage: insert_audio audio.wav video.dv\n"
			"or: insert_audio audio.wav <dv.file >dv.out\n");
		return -1;
	}
	if (argc == 2) {
		in_wav = fopen(argv[1], "r");
		in_vid = stdin;
		out_vid = stdout;
		have_pipes = 1;
	} else {
		in_wav = fopen(argv[1], "r");
		in_vid = fopen(argv[2], "r");
		out_vid = fopen(argv[2], "r+");
		have_pipes = 0;
	}
	if (!(in_vid != NULL && in_wav != NULL && out_vid != NULL)) {
		perror("Error opening files");
		exit(-1);
	}

	audio = parse_wave_header(in_wav);

	for (;;) {
		gotframe = read_frame(in_vid, frame_buf, &isPAL);
		bytesperframe = audio.bytespersecond / (isPAL ? 25 : 30);
		if (fread(wave_buf, 1, bytesperframe, in_wav) 
		    != bytesperframe) {
			if (have_pipes) {
				while (gotframe) {
					fwrite(frame_buf, 1,
					       isPAL ? 144000 : 120000,
					       out_vid);
					gotframe = read_frame(
						in_vid, frame_buf, &isPAL);
				}
			}
			return 0;
		}
		insert_audio(frame_buf, wave_buf, &audio, isPAL);
		fwrite(frame_buf, 1, isPAL ? 144000 : 120000, out_vid);
	}
}


