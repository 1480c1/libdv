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
#endif // HAVE_CONFIG_H

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

#define DV_PLAYER_OPT_VERSION         0
#define DV_PLAYER_OPT_DISABLE_AUDIO   1
#define DV_PLAYER_OPT_DISABLE_VIDEO   2
#define DV_PLAYER_OPT_NUM_FRAMES      3
#define DV_PLAYER_OPT_OSS_INCLUDE     4
#define DV_PLAYER_OPT_DISPLAY_INCLUDE 5
#define DV_PLAYER_OPT_DECODER_INCLUDE 6
#define DV_PLAYER_OPT_AUTOHELP        7
#define DV_PLAYER_NUM_OPTS            8

/* Book-keeping for mmap */
typedef struct dv_mmap_region_s {
  void   *map_start;  /* Start of mapped region (page aligned) */
  size_t  map_length; /* Size of mapped region */
  guint8 *data_start; /* Data we asked for */
} dv_mmap_region_t;

typedef struct {
  dv_decoder_t    *decoder;
  dv_display_t    *display;
  dv_oss_t        *oss;
  dv_mmap_region_t mmap_region;
  struct stat      statbuf;
  struct timeval   tv[3];
  gint             arg_disable_audio;
  gint             arg_disable_video;
  gint             arg_num_frames;
#ifdef HAVE_LIBPOPT
  struct poptOption option_table[DV_PLAYER_NUM_OPTS+1]; 
#endif // HAVE_LIBPOPT
} dv_player_t;

dv_player_t *
dv_player_new(void) 
{
  dv_player_t *result;
  
  if(!(result = calloc(1,sizeof(dv_player_t)))) goto no_mem;
  if(!(result->display = dv_display_new())) goto no_display;
  if(!(result->oss = dv_oss_new())) goto no_oss;
  if(!(result->decoder = dv_decoder_new())) goto no_decoder;

#if HAVE_LIBPOPT
  result->option_table[DV_PLAYER_OPT_VERSION] = (struct poptOption) {
    longName: "version", 
    shortName: 'v', 
    val: 'v', 
    descrip: "show playdv version number",
  }; // version

  result->option_table[DV_PLAYER_OPT_DISABLE_AUDIO] = (struct poptOption) {
    longName: "disable-audio", 
    arg:      &result->arg_disable_audio,
    descrip:  "skip audio decoding",
  }; // disable audio

  result->option_table[DV_PLAYER_OPT_DISABLE_VIDEO] = (struct poptOption) {
    longName: "disable-video", 
    descrip:  "skip video decoding",
    arg:      &result->arg_disable_video, 
  }; // disable video

  result->option_table[DV_PLAYER_OPT_NUM_FRAMES] = (struct poptOption) {
    longName:   "num-frames", 
    shortName:  'n', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_num_frames,
    argDescrip: "count",
    descrip:    "stop after <count> frames",
  }; // number of frames

  result->option_table[DV_PLAYER_OPT_OSS_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->oss->option_table,
    descrip: "Audio output options",
  }; // oss

  result->option_table[DV_PLAYER_OPT_DISPLAY_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->display->option_table,
    descrip: "Video output options",
  }; // display

  result->option_table[DV_PLAYER_OPT_DECODER_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->decoder->option_table,
    descrip: "Decoder options",
  }; // decoder

  result->option_table[DV_PLAYER_OPT_AUTOHELP] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     poptHelpOptions,
    descrip: "Help options",
  }; // autohelp

#endif // HAVE_LIBPOPT

  return(result);

 no_decoder:
  free(result->oss);
 no_oss:
  free(result->display);
 no_display:
  free(result);
  result = NULL;
 no_mem:
  return(result);
} // dv_player_new


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

int 
main(int argc,char *argv[]) 
{
  dv_player_t *dv_player;
  const char *filename;     /* name of input file */
  int fd;
  off_t offset = 0, eof;
  guint frame_count = 0;
  gint i;
  gdouble seconds = 0.0;
  gint16 *audio_buffers[4];
#if HAVE_LIBPOPT
  int rc;             /* return code from popt */
  poptContext optCon; /* context for parsing command-line options */

  if(!(dv_player = dv_player_new())) goto no_mem;

  /* Parse options using popt */
  optCon = poptGetContext(NULL, argc, (const char **)argv, dv_player->option_table, 0);
  poptSetOtherOptionHelp(optCon, "<filename>");

  while ((rc = poptGetNextOpt(optCon)) > 0) {
    switch (rc) {
    case 'v':
      goto display_version;
      break;
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
  if(fstat(fd, &dv_player->statbuf)) goto fstatfail;
  eof = dv_player->statbuf.st_size;

  dv_init(dv_player->decoder);

  dv_player->decoder->quality = dv_player->decoder->video->quality;

  /* Read in header of first frame to see how big frames are */
  mmap_unaligned(fd,0,header_size,&dv_player->mmap_region);
  if(MAP_FAILED == dv_player->mmap_region.map_start) goto map_failed;

  if(dv_parse_header(dv_player->decoder, dv_player->mmap_region.data_start)) goto header_parse_error;
  munmap_unaligned(&dv_player->mmap_region);

  eof -= dv_player->decoder->frame_size; // makes loop condition simpler

  if(!dv_player->arg_disable_video) {
    if(!dv_display_init (dv_player->display, &argc, &argv, 
			 dv_player->decoder->width, dv_player->decoder->height, 
			 dv_player->decoder->sampling, "playdv", "playdv")) goto no_display;
  } // if

  if(!dv_player->arg_disable_audio) {
    if(!dv_oss_init(dv_player->decoder->audio, dv_player->oss)) {
      dv_player->decoder->audio->num_channels = 0;
    } // if

    for(i=0; i < 4; i++) {
      if(!(audio_buffers[i] = malloc(DV_AUDIO_MAX_SAMPLES*sizeof(gint16)))) goto no_mem;
    } // for
  } // if

  gettimeofday(dv_player->tv+0,NULL);

  for(offset=0;
      offset <= eof;
      offset += dv_player->decoder->frame_size) {

    // Map the frame's data into memory
    mmap_unaligned(fd, offset, dv_player->decoder->frame_size, &dv_player->mmap_region);
    if(MAP_FAILED == dv_player->mmap_region.map_start) goto map_failed;

    // Parse and unshuffle audio
    if(!dv_player->arg_disable_audio) {
      dv_decode_full_audio(dv_player->decoder, dv_player->mmap_region.data_start, audio_buffers);
      dv_oss_play(dv_player->decoder->audio, dv_player->oss, audio_buffers);
    } // if

    if(!dv_player->arg_disable_video) {
      // Parse and decode video
      dv_decode_full_frame(dv_player->decoder, dv_player->mmap_region.data_start, 
			   dv_player->display->color_space, 
			   dv_player->display->pixels, 
			   dv_player->display->pitches);

      // Display
      dv_display_show(dv_player->display);
    } // if 

    frame_count++;
    if((dv_player->arg_num_frames > 0) && (frame_count >= dv_player->arg_num_frames)) {
      goto end_of_file;
    } // if 

    // Release the frame's data
    munmap_unaligned(&dv_player->mmap_region); 

  } // while

 end_of_file:
  gettimeofday(dv_player->tv+1,NULL);
  timersub(dv_player->tv+1,dv_player->tv+0,dv_player->tv+2);
  seconds = (double)dv_player->tv[2].tv_usec / 1000000.0; 
  seconds += dv_player->tv[2].tv_sec;
  fprintf(stderr,"Processed %d frames in %05.2f seconds (%05.2f fps)\n", 
	  frame_count, seconds, (double)frame_count/seconds);
  if(!dv_player->arg_disable_video) {
    dv_display_exit(dv_player->display);
  } // if
  if(!dv_player->arg_disable_audio) {
    dv_oss_close(dv_player->oss);
  } // if 
  exit(0);

 display_version:
  fprintf(stderr,"playdv: version %s, http://libdv.sourceforge.net/\n",
	  "CVS 01/13/2001");
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
  fprintf(stderr, "\nSpecify a single <filename> argument; e.g. pond.dv\n");
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
