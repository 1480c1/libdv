#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

void write_header_block(unsigned char* target, int ds, int isPAL)
{
	target[0] = 0x1f; /* header magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[3] = ( isPAL ? 0x80 : 0); /* FIXME: 0x3f */
	target[4] = 0x68; /* FIXME ? */
	target[5] = 0x78; /* FIXME ? */
	target[6] = 0x78; /* FIXME ? */
	target[7] = 0x78; /* FIXME ? */

	memset(target + 8, 0xff, 80 - 8);
}

inline void write_bcd(unsigned char* target, int val)
{
	*target = ((val / 10) << 4) + (val % 10);
}

void write_timecode_13(unsigned char* target, struct tm * now, int frame,
		       int isPAL)
{
	target[0] = 0x13;
	write_bcd(target+1, frame % (isPAL ? 25 : 30));
	write_bcd(target+2, now->tm_sec);
	write_bcd(target+3, now->tm_min);
	write_bcd(target+4, now->tm_hour);
}

/*
  60 ff ff 20 ff 61 33 c8 fd ff 62 ff d0 e1 01 63 ff b8 c6 c9
*/

void write_timecode_60(unsigned char* target, struct tm * now)
{
	target[0] = 0x60;
	target[1] = 0xff;
	target[2] = 0xff;
	write_bcd(target + 3, (now->tm_year + 1900) / 100); 
                              /* FIXME: Is this true? 
				 I've doubts since newyears day ;-) */
	target[4] = 0xff;
}

void write_timecode_61(unsigned char* target, struct tm * now)
{
	target[0] = 0x61; /* FIXME: What's this? */

	target[1] = 0x33;
	target[2] = 0xc8;
	target[3] = 0xfd;

	target[4] = 0xff;
}

void write_timecode_62(unsigned char* target, struct tm * now)
{
	target[0] = 0x62;
	target[1] = 0xff;
	write_bcd(target + 2, now->tm_mday);
	write_bcd(target + 3, now->tm_mon);
	write_bcd(target + 4, now->tm_year % 100);
}
void write_timecode_63(unsigned char* target, struct tm * now)
{
	target[0] = 0x63;
	target[1] = 0xff;
	write_bcd(target + 2, now->tm_sec);
	write_bcd(target + 3, now->tm_min);
	write_bcd(target + 4, now->tm_hour);
}

void write_subcode_blocks(unsigned char* target, int ds, int frame, 
			  struct tm * now, int isPAL)
{
	static int block_count = 0;

	memset(target, 0xff, 2*80);

	target[0] = 0x3f; /* subcode magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[80 + 0] = 0x3f; /* subcode magic */
	target[80 + 1] = 0x07 | (ds << 4);
	target[80 + 2] = 0x01;
	
	target[5] = target[80 + 5] = 0xff;

	if (ds >= 6) {
		target[3] = 0x80 | (block_count >> 8);
		target[4] = block_count;

		target[80 + 3] = 0x80 | (block_count >> 8);
		target[80 + 4] = block_count + 6;

		write_timecode_13(target + 6, now, frame, isPAL);
		write_timecode_13(target + 80 + 6, now, frame, isPAL);

		write_timecode_62(target + 6 + 8, now);
		write_timecode_62(target + 80 + 6 + 8, now);

		write_timecode_63(target + 6 + 2*8, now);
		write_timecode_63(target + 80 + 6 + 2*8, now);

		write_timecode_13(target + 6 + 3*8, now, frame, isPAL);
		write_timecode_13(target + 80 + 6 + 3*8, now, frame, isPAL);

		write_timecode_62(target + 6 + 4*8, now);
		write_timecode_62(target + 80 + 6 + 4*8, now);

		write_timecode_63(target + 6 + 5*8, now);
		write_timecode_63(target + 80 + 6 + 5*8, now);
	} else {
		target[3] = (block_count >> 8);
		target[4] = block_count;

		target[80 + 3] = (block_count >> 8);
		target[80 + 4] = block_count + 6;
		
	}
	block_count += 0x20;
	block_count &= 0xfff;
}

void write_vaux_blocks(unsigned char* target, int ds, struct tm* now)
{
	memset(target, 0xff, 3*80);

	target[0] = 0x5f; /* vaux magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[0 + 80] = 0x5f; /* vaux magic */
	target[1 + 80] = 0x07 | (ds << 4);
	target[2 + 80] = 0x01;

	target[0 + 2*80] = 0x5f; /* vaux magic */
	target[1 + 2*80] = 0x07 | (ds << 4);
	target[2 + 2*80] = 0x02;


	if ((ds & 1) == 0) {
		if (ds == 0) {
			target[3] = 0x70; /* FIXME: What's this? */
			target[4] = 0xc5;
			target[5] = 0x41; /* 42 ? */
			target[6] = 0x20;
			target[7] = 0xff;
			target[8] = 0x71;
			target[9] = 0xff;
			target[10] = 0x7f;
			target[11] = 0xff;
			target[12] = 0xff;
			target[13] = 0x7f;
			target[14] = 0xff;
			target[15] = 0xff;
			target[16] = 0x38;
			target[17] = 0x81;
		}
	} else {
		write_timecode_60(target + 3, now);
		write_timecode_61(target + 3 + 5, now);
		write_timecode_62(target + 3 + 2*5, now);
		write_timecode_63(target + 3 + 3*5, now);
	}
	write_timecode_60(target + 2*80+ 48, now);
	write_timecode_61(target + 2*80+ 48 + 5, now);
	write_timecode_62(target + 2*80+ 48 + 2*5, now);
	write_timecode_63(target + 2*80+ 48 + 3*5, now);
}

void write_video_headers(unsigned char* target, int frame, int ds)
{
	int i,j, z;
	z = 0;
	for(i = 0; i < 9; i++) {
		target += 80;
		for (j = 1; j < 16; j++) { /* j = 0: audio blocks */
			target[0] = 0x90 | ((frame + 0xb) % 12);
			target[1] = 0x07 | (ds << 4);
			target[2] = z++;
			target += 80;
		}
	}
}

void write_audio_headers(unsigned char* target, int frame, int ds)
{
	int i, z;
	z = 0;
	for(i = 0; i < 9; i++) {
		memset(target, 0xff, 80);

		target[0] = 0x70 | ((frame + 0xb) % 12);
		target[1] = 0x07 | (ds << 4);
		target[2] = z++;

		target += 16 * 80;
	}
}


void write_info_blocks(unsigned char* target, int frame, int isPAL,
		       time_t * now)
{
	int numDIFseq;
	int ds;
	struct tm * now_t = NULL;

	numDIFseq = isPAL ? 12 : 10;

	if (frame % (isPAL ? 25 : 30) == 0) {
		(*now)++;
	}

	now_t = localtime(now);

	for (ds = 0; ds < numDIFseq; ds++) { 
		write_header_block(target, ds, isPAL);
		target +=   1 * 80;
		write_subcode_blocks(target, ds, frame, now_t, isPAL);
		target +=   2 * 80;
		write_vaux_blocks(target, ds, now_t);
		target +=   3 * 80;
		write_video_headers(target, frame, ds);
		write_audio_headers(target, frame, ds);
		target += 144 * 80;
	}
}

int read_frame(FILE* in_fp, unsigned char* frame_buf, int * isPAL)
{
	if (fread(frame_buf, 1, 120000, in_fp ) != 120000) {
		return 0;
	}

	*isPAL = (frame_buf[3] & 0x80);

	if (*isPAL) {
		if (fread(frame_buf + 120000, 1, 144000 - 120000, in_fp) !=
		    144000 - 120000) {
			return 0;
		}
	}
	return 1;
}

int main(int argc, const char** argv)
{
	unsigned char frame_buf[144000];
	int isPAL;
	int frame_count = 0;
	time_t t = time(NULL);

	while (read_frame(stdin, frame_buf, &isPAL)) {
		write_info_blocks(frame_buf, frame_count++, isPAL, &t);
		fwrite(frame_buf, 1, isPAL ? 144000 : 120000, stdout);
	}
	return 0;
}
