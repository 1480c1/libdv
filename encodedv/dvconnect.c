/* 
 *  dvconnect.c
 *
 *     Copyright (C) Peter Schlaile - May 2001
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#if HAVE_LIBPOPT
#include <popt.h>
#endif

#include <malloc.h>

#define CIP_N_NTSC 2436
#define CIP_D_NTSC 38400
#define CIP_N_PAL 1
#define CIP_D_PAL 16

#define TARGETBUFSIZE   320
#define MAX_PACKET_SIZE 512
#define VBUF_SIZE      (320*512)

#define VIDEO1394_MAX_SIZE 0x4000000

enum {
	VIDEO1394_BUFFER_FREE = 0,
	VIDEO1394_BUFFER_QUEUED,
	VIDEO1394_BUFFER_READY
};

enum {
	VIDEO1394_LISTEN_CHANNEL = 0,
	VIDEO1394_UNLISTEN_CHANNEL,
	VIDEO1394_LISTEN_QUEUE_BUFFER,
	VIDEO1394_LISTEN_WAIT_BUFFER,
	VIDEO1394_TALK_CHANNEL,
	VIDEO1394_UNTALK_CHANNEL,
	VIDEO1394_TALK_QUEUE_BUFFER,
	VIDEO1394_TALK_WAIT_BUFFER
};

#define VIDEO1394_SYNC_FRAMES          0x00000001
#define VIDEO1394_INCLUDE_ISO_HEADERS  0x00000002
#define VIDEO1394_VARIABLE_PACKET_SIZE 0x00000004

struct video1394_mmap
{
	unsigned int channel;
	unsigned int sync_tag;
	unsigned int nb_buffers;
	unsigned int buf_size;
        /* For VARIABLE_PACKET_SIZE: Maximum packet size */
	unsigned int packet_size; 
	unsigned int fps;
	unsigned int flags;
};

/* For TALK_QUEUE_BUFFER with VIDEO1394_VARIABLE_PACKET_SIZE use */
struct video1394_queue_variable
{
	unsigned int channel;
	unsigned int buffer;
	unsigned int* packet_sizes; /* Buffer of size: buf_size / packet_size*/
};

struct video1394_wait
{
	unsigned int channel;
	unsigned int buffer;
};

static unsigned char* p_outbuf;
static unsigned char* p_out;
static int cap_start_frame = 0;
static int cap_num_frames = 0xfffffff;
static int cap_verbose_mode;
static int frames_captured = 0;
static int broken_frames = 0;

void handle_packet(unsigned char* data, int len)
{
	int start_of_frame = 0;
	static int frame_size = 0;
	static int isPAL = 0;
	static int found_first_block = 0;

        switch (len) {
        case 488: 
		if (data[12] == 0x1f && data[13] == 0x07) {
			start_of_frame = 1;
		}

		if (start_of_frame) {
			if (!found_first_block) {
				if (!cap_start_frame) {
					found_first_block = 1;
				} else {
					cap_start_frame--;
					if (cap_verbose_mode) {
						fprintf(stderr,
							"Skipped frame %8d\r",
							cap_start_frame);
					}
					return;
				}
				frame_size = 0;
			}
			if (!cap_num_frames) {
				return;
			}
			cap_num_frames--;
			
			if (frame_size) {
				if ((isPAL && frame_size != 144000)
				    || (!isPAL && frame_size != 120000)) {
					fprintf(stderr, 
						"Broken frame %8d "
						"(size = %d)!\n",
						frames_captured, frame_size);
					broken_frames++;
				}
				if (cap_verbose_mode) {
					fprintf(stderr, 
						"Captured frame %8d\r",
						frames_captured);
				}
				frames_captured++;
				frame_size = 0;
			}
			isPAL = (data[15] & 0x80);
		}

		if (found_first_block && cap_num_frames != 0) {
			memcpy(p_out, data + 12, 480);
			p_out += 480;
			frame_size += 480;
		}

                break;
        case 8:
                break;
        default:
		fprintf(stderr, "Penguin on the bus? (packet size = %d!)\n",
			len);
                break;
        }
}

void finish_block(FILE* dst_fp)
{
	if (p_out != p_outbuf) {
		fwrite(p_outbuf, 1, p_out - p_outbuf, dst_fp);
		p_out = p_outbuf;
	}
}

int capture_raw(const char* filename, int channel, int nbuffers,
		int start_frame, int end_frame, int verbose_mode)
{
        int viddev;
        unsigned char *recv_buf;
        struct video1394_mmap v;
        struct video1394_wait w;
        FILE* dst_fp;
	int unused_buffers;
        unsigned char outbuf[2*65536];
        int outbuf_used = 0;

        p_outbuf = (unsigned char*) malloc(VBUF_SIZE);
        p_out = p_outbuf;
	cap_start_frame = start_frame;
	cap_num_frames = end_frame - start_frame;
	cap_verbose_mode = verbose_mode;
	
	if (!filename || strcmp(filename, "-") == 0) {
		dst_fp = stdout;
	} else {
		dst_fp = fopen(filename, "rb");
		if (!dst_fp) {
			perror("fopen input file");
			return(-1);
		}
	}

        if ((viddev = open("/dev/video1394", O_RDWR)) < 0) {
		perror("open /dev/video1394");
                return -1;
        }
                      
        v.channel = channel;
        v.sync_tag = 0;
        v.nb_buffers = nbuffers;
        v.buf_size = VBUF_SIZE; 
        v.packet_size = MAX_PACKET_SIZE;
        v.flags = VIDEO1394_INCLUDE_ISO_HEADERS;
        w.channel = v.channel;

        if (ioctl(viddev, VIDEO1394_LISTEN_CHANNEL, &v)<0) {
                perror("VIDEO1394_LISTEN_CHANNEL");
                return -1;
        }

        if ((recv_buf = (unsigned char *) mmap(
                0, v.nb_buffers*v.buf_size, PROT_READ|PROT_WRITE,
                MAP_SHARED, viddev, 0)) == (unsigned char *)-1) {
                perror("mmap videobuffer");
                return -1;
        }

        unused_buffers = v.nb_buffers;
        w.buffer = 0;
       
        while (cap_num_frames != 0) {
		struct video1394_wait wcopy;
		unsigned char * curr;
                int ofs;

                while (unused_buffers--) {
                        unsigned char * curr = recv_buf+ v.buf_size * w.buffer;

                        memset(curr, 0, v.buf_size);
                        
                        if (ioctl(viddev,VIDEO1394_LISTEN_QUEUE_BUFFER,&w)<0) {
                                perror("VIDEO1394_LISTEN_QUEUE_BUFFER");
                        }
                        w.buffer++;
                        w.buffer %= v.nb_buffers;
                }
                wcopy = w;
                if (ioctl(viddev, VIDEO1394_LISTEN_WAIT_BUFFER, &wcopy) < 0) {
                        perror("VIDEO1394_LISTEN_WAIT_BUFFER");
                }
                curr = recv_buf + v.buf_size * w.buffer;
                ofs = 0;

                while (ofs < VBUF_SIZE) {
                        while (outbuf_used < 4 && ofs < VBUF_SIZE) {
                                outbuf[outbuf_used++] = curr[ofs++];
                        }
                        if (ofs != VBUF_SIZE) {
                                int len = outbuf[2] + (outbuf[3] << 8) + 8;
                                if (ofs + len - outbuf_used > VBUF_SIZE) {
                                        memcpy(outbuf + outbuf_used, curr+ofs, 
                                               VBUF_SIZE - ofs);
                                        outbuf_used += VBUF_SIZE - ofs;
                                        ofs = VBUF_SIZE;
                                } else {
                                        memcpy(outbuf + outbuf_used,
                                               curr + ofs, len - outbuf_used);
                                        ofs += len - outbuf_used;
                                        outbuf_used = 0;
                                        handle_packet(outbuf, len - 8);
                                }
                        }
                }

                finish_block(dst_fp);

                unused_buffers = 1;
        }

	if (broken_frames) {
		fprintf(stderr, "\nCaptured %d broken frames!\n",
			broken_frames);
	}

        munmap(recv_buf, v.nb_buffers * v.buf_size);
        
        if (ioctl(viddev, VIDEO1394_UNLISTEN_CHANNEL, &v.channel)<0) {
                perror("VIDEO1394_UNLISTEN_CHANNEL");
        }

        close(viddev);
	if (dst_fp != stdout) {
		fclose(dst_fp);
	}

        return 0;
}

int read_frame(FILE* src_fp, unsigned char* frame, int* isPAL)
{
        if (fread(frame, 1, 120000, src_fp) != 120000) {
                return -1;
        }
	*isPAL = (frame[3] & 0x80);

	if (*isPAL) {
		if (fread(frame + 120000, 1, 144000 - 120000, src_fp)
		    != 144000 - 120000) {
			return -1;
		}
	}
	return 0;
}

static int fill_buffer(FILE* src_fp, unsigned char* targetbuf,
                       unsigned int * packet_sizes)
{
        unsigned char frame[144000]; /* PAL is large enough... */
	int isPAL;
	int frame_size;
        unsigned long vdata = 0;
        int i;
        static unsigned char continuity_counter = 0;
	static unsigned int cip_counter = 0;
	static unsigned int cip_n = 0;
	static unsigned int cip_d = 0;

	if (read_frame(src_fp, frame, &isPAL) < 0) {
		return -1;
	}

	frame_size = isPAL ? 144000 : 120000;

	if (cip_counter == 0) {
		if (!isPAL) {
			cip_n = CIP_N_NTSC;
			cip_d = CIP_D_NTSC;
		} else {
			cip_n = CIP_N_PAL;
			cip_d = CIP_D_PAL;
		}
		cip_counter = cip_n;
	}

        for (i = 0; i < TARGETBUFSIZE && vdata < frame_size; i++) {
                unsigned char* p = targetbuf;
                int want_sync = 0;
		cip_counter += cip_n;

		if (cip_counter > cip_d) {
			want_sync = 1;
			cip_counter -= cip_d;
		}

                *p++ = 0x01; /* Source node ID ! */
                *p++ = 0x78; /* Packet size in quadlets (480 / 4) */
                *p++ = 0x00;
                *p++ = continuity_counter;
                
                *p++ = 0x80; /* const */
                *p++ = 0x80; /* const */

                *p++ = 0xff; /* timestamp */
                *p++ = 0xff; /* timestamp */
                
                /* Timestamping is now done in the kernel driver! */
                if (!want_sync) { /* video data */
                        continuity_counter++;

                        memcpy(p, frame + vdata, 480);
                        p += 480;
                        vdata += 480;
                }

                *packet_sizes++ = p - targetbuf;
                targetbuf += MAX_PACKET_SIZE;
        }
        *packet_sizes++ = 0;

        return 0;
}

int send_raw(const char* filename, int channel, int nbuffers, int start_frame,
	     int end_frame, int verbose_mode)
{
        int viddev;
        unsigned char *send_buf;
        struct video1394_mmap v;
        struct video1394_queue_variable w;
        FILE* src_fp;
	int unused_buffers;
	int got_frame;
        unsigned int packet_sizes[321];

	if (!filename || strcmp(filename, "-") == 0) {
		src_fp = stdin;
	} else {
		src_fp = fopen(filename, "rb");
		if (!src_fp) {
			perror("fopen input file");
			return -1;
		}
	}

	if (start_frame > 0) {
		int isPAL,i;
		unsigned char frame[144000]; /* PAL is large enough... */

		for (i = 0; i < start_frame; i++) {
			if (verbose_mode) {
				fprintf(stderr, "Skipped frame %8d\r",
					start_frame - i);
			}
			if (read_frame(src_fp, frame, &isPAL) < 0) {
				return -1;
			}
		}
	}

        if ((viddev = open("/dev/video1394",O_RDWR)) < 0) {
		perror("open /dev/video1394");
                return -1;
        }
                      
        v.channel = channel;
        v.sync_tag = 0;
        v.nb_buffers = nbuffers;
        v.buf_size = TARGETBUFSIZE * MAX_PACKET_SIZE; 
        v.packet_size = MAX_PACKET_SIZE;
        v.flags = VIDEO1394_VARIABLE_PACKET_SIZE;
        w.channel = v.channel;

        if (ioctl(viddev, VIDEO1394_TALK_CHANNEL, &v)<0) {
                perror("VIDEO1394_TALK_CHANNEL");
                return -1;
        }
        if ((send_buf = (unsigned char *) mmap(
                0, v.nb_buffers * v.buf_size, PROT_READ|PROT_WRITE,
                MAP_SHARED, viddev, 0)) == (unsigned char *)-1) {
                perror("mmap videobuffer");
                return -1;
        }

        unused_buffers = v.nb_buffers;
        w.buffer = 0;
        got_frame = 1;
        w.packet_sizes = packet_sizes;
	memset(packet_sizes, 0, sizeof(packet_sizes));

        for (;start_frame < end_frame;) {
                while (unused_buffers--) {
                        got_frame = (fill_buffer(
                                src_fp, send_buf + w.buffer * v.buf_size,
                                packet_sizes) < 0) ? 0 : 1;

                        if (!got_frame) {
                                break;
                        }
                        if (ioctl(viddev, VIDEO1394_TALK_QUEUE_BUFFER, &w)<0) {
                                perror("VIDEO1394_TALK_QUEUE_BUFFER");
                        }
			if (verbose_mode) {
				fprintf(stderr, "Sent frame %8d\r",
					start_frame);
			}
                        w.buffer ++;
                        w.buffer %= v.nb_buffers;
			start_frame++;
                }
                if (!got_frame) {
                        break;
                }
                if (ioctl(viddev, VIDEO1394_TALK_WAIT_BUFFER, &w) < 0) {
                        perror("VIDEO1394_TALK_WAIT_BUFFER");
                }
                unused_buffers = 1;
        }

        w.buffer = (v.nb_buffers + w.buffer - 1) % v.nb_buffers;

        if (ioctl(viddev, VIDEO1394_TALK_WAIT_BUFFER, &w) < 0) {
                perror("VIDEO1394_TALK_WAIT_BUFFER");
        }

        munmap(send_buf, v.nb_buffers * v.buf_size);
        
        if (ioctl(viddev, VIDEO1394_UNTALK_CHANNEL, &v.channel)<0) {
                perror("VIDEO1394_UNTALK_CHANNEL");
        }

        close(viddev);
	if (src_fp != stdin) {
		fclose(src_fp);
	}

        return 0;
}

#define DV_CONNECT_OPT_VERSION         0
#define DV_CONNECT_OPT_SEND            1
#define DV_CONNECT_OPT_VERBOSE         2
#define DV_CONNECT_OPT_CHANNEL         3
#define DV_CONNECT_OPT_BUFFERS         4
#define DV_CONNECT_OPT_START_FRAME     5
#define DV_CONNECT_OPT_END_FRAME       6
#define DV_CONNECT_OPT_AUTOHELP        7
#define DV_CONNECT_NUM_OPTS            8


int main(int argc, const char** argv)
{
#if HAVE_LIBPOPT
	const char* filename;
	int verbose_mode = 0;
	int send_mode = 0;
	int channel = 63;
	int buffers = 8;
	int start = 0;
	int end = 0xfffffff;

        struct poptOption option_table[DV_CONNECT_NUM_OPTS+1]; 
        int rc;             /* return code from popt */
        poptContext optCon; /* context for parsing command-line options */

        option_table[DV_CONNECT_OPT_VERSION] = (struct poptOption) {
                longName: "version", 
                val: 'v', 
                descrip: "show dvconnect version number"
        }; /* version */

        option_table[DV_CONNECT_OPT_SEND] = (struct poptOption) {
                longName:   "send", 
                shortName:  's', 
                arg:        &send_mode,
                descrip:    "send data instead of capturing"
        }; /* send mode */
	
        option_table[DV_CONNECT_OPT_VERBOSE] = (struct poptOption) {
                longName:   "verbose", 
                shortName:  'v', 
                arg:        &verbose_mode,
                descrip:    "show status information while receiving / sending"
        }; /* verbose mode */

        option_table[DV_CONNECT_OPT_CHANNEL] = (struct poptOption) {
                longName:   "channel", 
                shortName:  'c', 
                argInfo:    POPT_ARG_INT, 
                arg:        &channel,
                argDescrip: "number",
                descrip:    "channel to send / receive on "
		"(range: 0 - 63, default = 63)"
        }; /* channel */

        option_table[DV_CONNECT_OPT_BUFFERS] = (struct poptOption) {
                longName:   "buffers", 
                shortName:  'b', 
                argInfo:    POPT_ARG_INT, 
                arg:        &buffers,
                argDescrip: "number",
                descrip:    "number of video frame buffers to use. default = 8"
        }; /* buffers */

        option_table[DV_CONNECT_OPT_START_FRAME] = (struct poptOption) {
                longName:   "start-frame", 
                argInfo:    POPT_ARG_INT, 
                arg:        &start,
                argDescrip: "count",
                descrip:    "start at <count> frame (defaults to 0)"
        }; /* start-frame */

        option_table[DV_CONNECT_OPT_END_FRAME] = (struct poptOption) {
                longName:   "end-frame", 
                shortName:  'e', 
                argInfo:    POPT_ARG_INT, 
                arg:        &end,
                argDescrip: "count",
                descrip:    "end at <count> frame (defaults to unlimited)"
        }; /* end-frames */

        option_table[DV_CONNECT_OPT_AUTOHELP] = (struct poptOption) {
                argInfo: POPT_ARG_INCLUDE_TABLE,
                arg:     poptHelpOptions,
                descrip: "Help options",
        }; /* autohelp */

        option_table[DV_CONNECT_NUM_OPTS] = (struct poptOption) { 
                NULL, 0, 0, NULL, 0 };

        optCon = poptGetContext(NULL, argc, 
                                (const char **)argv, option_table, 0);
        poptSetOtherOptionHelp(optCon, "<raw dv file or - for stdin/stdout>");

        while ((rc = poptGetNextOpt(optCon)) > 0) {
                switch (rc) {
                case 'v':
                        fprintf(stderr,"dvconnect: version %s, "
                                "http://libdv.sourceforge.net/\n",
                                "CVS 05/06/2001");
                        return 0;
                        break;
                default:
                        break;
                }
        }

        if (rc < -1) {
                /* an error occurred during option processing */
                fprintf(stderr, "%s: %s\n",
                        poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                        poptStrerror(rc));
		return -1;
        }

	if (channel < 0 || channel > 63) {
		fprintf(stderr, "Invalid channel chosen: %d\n"
			"Should be in the range 0 - 63\n", channel);
		return -1;
	}
	if (buffers < 0) {
		fprintf(stderr, "Number of buffers should be > 0!\n");
	        return -1;
	}

        filename = poptGetArg(optCon);

	if (send_mode) {
		send_raw(filename, channel, buffers, start, end, verbose_mode);
	} else {
		capture_raw(filename, channel, buffers, start, end, 
			    verbose_mode);
	}
	return 0;
#else
#warning dvconnect can not work without libpopt!	

#endif
}














