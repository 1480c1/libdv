/* 
 *  playdv.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  decoder.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#if HAVE_LIBPOPT
#include <popt.h>
#endif // HAVE_LIBPOPT

#include "dv.h"
#include "display.h"
#include "oss.h"
#include "bitstream.h"
#include "place.h"

/* Book-keeping for mmap */

typedef struct dv_mmap_region_s {
  void   *map_start;  /* Start of mapped region (page aligned) */
  size_t  map_length; /* Size of mapped region */
  guint8 *data_start; /* Data we asked for */
} dv_mmap_region_t;

/* I decided to try to use mmap for reading the input.  I got a slight
 * (about %5-10) performance improvement */

void
mmap_unaligned(int fd, off_t offset, size_t length, dv_mmap_region_t *mmap_region) {
  size_t real_length;
  off_t  real_offset;
  size_t page_size;
  size_t start_padding;
  void *real_start;

  page_size = getpagesize();
  start_padding = offset % page_size;
  real_offset = (offset / page_size) * page_size;
  real_length = real_offset + start_padding + length;
  real_start = mmap(0, real_length, PROT_READ, MAP_SHARED, fd, real_offset);

  mmap_region->map_start = real_start;
  mmap_region->map_length = real_length;
  mmap_region->data_start = real_start + start_padding;
} // mmap_unaligned

int
munmap_unaligned(dv_mmap_region_t *mmap_region) {
  return(munmap(mmap_region->map_start,mmap_region->map_length));
} // munmap_unaligned

static dv_decoder_t dv;
static dv_mmap_region_t mmap_region;
static struct stat statbuf;
static struct timeval tv[3];

static gint arg_disable_audio;
static gint arg_disable_video;
static gint arg_block_quality = 3;
static gint arg_monochrome;
static gint arg_audio_frequency;
static gint arg_audio_quantization;
static gint arg_display;
static gint arg_num_frames;
static gint arg_video_system;
static gchar * arg_audio_file;
static gchar * arg_audio_device;

#if HAVE_LIBPOPT
struct poptOption optionsTable[] = {
  { longName: "version", 
    shortName: 'v', 
    argInfo: POPT_ARG_NONE, 
    arg: NULL,
    val: 'v', 
    descrip: "show playdv version number",
    argDescrip: NULL
  },
  { longName: "disable-audio", 
    shortName: '\0', 
    argInfo: POPT_ARG_NONE, 
    arg: &arg_disable_audio,
    val: 0, 
    descrip: "skip audio decoding",
    argDescrip: NULL
  },
  { longName: "disable-video", 
    shortName: '\0', 
    argInfo: POPT_ARG_NONE, 
    arg: &arg_disable_video,
    val: 0, 
    descrip: "skip video decoding",
    argDescrip: NULL
  },
  { longName: "quality", 
    shortName: 'q', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_block_quality,
    val: 0, 
    descrip: "set video quality level (coeff. parsing):"
    "  1=DC and no ACs,"
    " 2=DC and single-pass for ACs ,"
    " 3=DC and multi-pass for ACs [default]",
    argDescrip: "(1|2|3)"
  },
  { longName: "monochrome", 
    shortName: 'm', 
    argInfo: POPT_ARG_NONE, 
    arg: &arg_monochrome,
    val: 0, 
    descrip: "skip decoding of color blocks",
    argDescrip: NULL
  },
  { longName: "frequency", 
    shortName: 'f', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_audio_frequency,
    val: 0, 
    descrip: "audio frequency: 0=autodetect [default], 1=32 kHz, 2=44.1 kHz, 3=48 kHz",
    argDescrip: "(0|1|2|3)"
  },
  { longName: "quantization", 
    shortName: 'Q', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_audio_quantization,
    val: 0, 
    descrip: "force audio quantization: 0=autodetect [default], 1=12 bit, 2=16bit",
    argDescrip: "(0|1|2)"
  },
  { longName: "num-frames", 
    shortName: 'n', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_num_frames,
    val: 0, 
    descrip: "stop after <count> frames",
    argDescrip: "count",
  },
  { longName: "video system", 
    shortName: 'V', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_video_system,
    val: 0, 
    descrip: "select video system:" 
    "0=autoselect [default]," 
    " 1=525/60 4:1:1 (NTSC),"
    " 2=625/50 4:2:0 (PAL,IEC 61834 DV),"
    " 3=625/50 4:1:1 (PAL,SMPTE 314M DV)",
    argDescrip: "(0|1|2)",
  },

  { longName: "display", 
    shortName: 'd', 
    argInfo: POPT_ARG_INT, 
    arg: &arg_display,
    val: 0, 
    descrip: "video display method: 0=autoselect [default], 1=gtk, 2=Xv, 3=SDL",
    argDescrip: "(0|1|2|3)",
  },
  { longName: "audio-device", 
    shortName: '\0', 
    argInfo: POPT_ARG_STRING, 
    arg: &arg_audio_device,
    val: 0, 
    descrip: "target audio device; e.g. /dev/dsp [default]",
    argDescrip: "devicename",
  },
  { longName: "audio-file", 
    shortName: '\0', 
    argInfo: POPT_ARG_STRING, 
    arg: &arg_audio_file,
    val: 0, 
    descrip: "send raw decoded audio to file, skipping audio ioctls",
    argDescrip: "filename",
  },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0, 0 }
}; /* optionsTable */
#endif // HAVE_LIBPOPT

int 
main(int argc,char *argv[]) 
{
  const char *filename;     /* name of input file */
  dv_display_t *dv_dpy = NULL;
  dv_oss_t      dv_oss;
  int fd;
  off_t offset = 0, eof;
  guint frame_count = 0;
  gint i;
  gdouble seconds;
  gboolean benchmark_mode = FALSE, audio_present;
  gint16 *audio_buffers[4];
#if HAVE_LIBPOPT
  int rc;             /* return code from popt */
  poptContext optCon; /* context for parsing command-line options */

  /* Parse options using popt */
  optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "<filename>");

  while ((rc = poptGetNextOpt(optCon)) > 0) {
    switch (rc) {
      default:
      break;
    } /* switch */
  } /* while */

  if (rc < -1) goto bad_arg;

  filename = poptGetArg(optCon);
  if((filename == NULL) || !(poptPeekArg(optCon) == NULL)) goto bad_filename;
  poptFreeContext(optCon);
#else
  /* No popt, no usage and no options!  HINT: get popt if you don't
   * have it yet, it's at: ftp://ftp.redhat.com/pub/redhat/code/popt 
   */
  filename = argv[1];
#endif // HAVE_LIBOPT

  /* Open the input file, do fstat to get it's total size */
  if(-1 == (fd = open(filename,O_RDONLY))) goto openfail;
  if(fstat(fd, &statbuf)) goto fstatfail;
  eof = statbuf.st_size;

  dv_init(&dv);
  dv.quality = DV_QUALITY_BEST;

  /* Read in header of first frame to see how big frames are */
  mmap_unaligned(fd,0,header_size,&mmap_region);
  if(MAP_FAILED == mmap_region.map_start) goto map_failed;

  if(dv_parse_header(&dv, mmap_region.data_start)) goto header_parse_error;
  munmap_unaligned(&mmap_region);

  eof -= dv.frame_size; // makes loop condition simpler

  dv_dpy = dv_display_init (&argc, &argv, dv.width, dv.height, dv.sampling, "playdv", "playdv");
  if(!dv_dpy) goto no_display;

  if(!dv_oss_init(&dv.audio, &dv_oss)) 
    dv.audio.num_channels = 0;

  for(i=0; i < dv.audio.num_channels; i++) {
    if(!(audio_buffers[i] = malloc(DV_AUDIO_MAX_SAMPLES*sizeof(gint16)))) goto no_mem;
  }

  gettimeofday(tv+0,NULL);

  for(offset=0;
      offset <= eof;
      offset += dv.frame_size) {

    // Map the frame's data into memory
    mmap_unaligned(fd, offset, dv.frame_size, &mmap_region);
    if(MAP_FAILED == mmap_region.map_start) goto map_failed;

    // Parse and unshuffle audio
    if(dv.audio.num_channels) {
      audio_present = dv_decode_full_audio(&dv, mmap_region.data_start, audio_buffers);
      dv_oss_play(&dv.audio, &dv_oss, audio_buffers);
    } // if

    // Parse and decode video
    dv_decode_full_frame(&dv, mmap_region.data_start, dv_dpy->color_space, dv_dpy->pixels, dv_dpy->pitches);

    // Display

    dv_display_show(dv_dpy);

    frame_count++;
    if(benchmark_mode && (frame_count >= 150)) break;

    // Release the frame's data
    munmap_unaligned(&mmap_region); 

  } // while

  gettimeofday(tv+1,NULL);
  timersub(tv+1,tv+0,tv+2);
  seconds = (double)tv[2].tv_usec / 1000000.0; 
  seconds += tv[2].tv_sec;
  fprintf(stderr,"Displayed %d frames in %05.2f seconds (%05.2f fps)\n", frame_count, seconds, (double)frame_count/seconds);
  dv_display_exit(dv_dpy);
  dv_oss_close(&dv_oss);
  exit(0);

  /* Error handling section */
 bad_arg:
  /* an error occurred during option processing */
  fprintf(stderr, "%s: %s\n",
	  poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
	  poptStrerror(rc));
  exit(-1);

#if HAVE_LIBPOPT
 bad_filename:
  poptPrintUsage(optCon, stderr, 0);
  fprintf(stderr, "Specify a single <filename> argument; e.g. pond.dv\n");
  exit(-1);
#endif
 no_display:
  exit(-1);
 openfail:
  perror("open:");
  exit(-1);
 fstatfail:
  perror("fstat:");
  exit(-1);
 map_failed:
  perror("mmap:");
  exit(-1);
 header_parse_error:
  fprintf(stderr,"Parser error reading first header\n");
  exit(-1);
 no_mem:
  fprintf(stderr,"Out of memory\n");
  exit(-1);
} // main
