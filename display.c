/* 
 *  display.c
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

/* Most of this file is derived from patches 101018 and 101136 submitted by
 * Stefan Lucke <lucke@berlin.snafu.de> */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <dv_types.h>
#include "util.h"
#include "display.h"

#if HAVE_LIBXV
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if HAVE_LIBPOPT
#include <popt.h>
#endif

static int dv_display_SDL_init(dv_display_t *dv_dpy, gchar *w_name, gchar *i_name);
static gboolean dv_display_gdk_init(dv_display_t *dv_dpy, gint *argc, gchar ***argv);
static gint dv_display_Xv_init(dv_display_t *dv_dpy, gchar *w_name, gchar *i_name);

#if HAVE_SDL
static void dv_center_window(SDL_Surface *screen);
#endif

#if HAVE_LIBXV
static void dv_display_event (dv_display_t *dv_dpy);
#endif 


#if HAVE_LIBPOPT
static void
dv_display_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_display_t *display = (dv_display_t *)data;

  if((display->arg_display < 0) || (display->arg_display > 3)) {
    dv_opt_usage(con, display->option_table, DV_DISPLAY_OPT_METHOD);
  } // if
  
} // dv_display_popt_callback 
#endif // HAVE_LIBPOPT

dv_display_t *
dv_display_new(void) 
{
  dv_display_t *result;

  result = calloc(1,sizeof(dv_display_t));
  if(!result) goto no_mem;

#if HAVE_LIBPOPT
  result->option_table[DV_DISPLAY_OPT_METHOD] = (struct poptOption) {
    longName:   "display", 
    shortName:  'd', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_display,
    descrip:    "video display method: 0=autoselect [default], 1=gtk, 2=Xv, 3=SDL",
    argDescrip: "(0|1|2|3)",
  }; // display method

  result->option_table[DV_DISPLAY_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_display_popt_callback,
    descrip: (char *)result, // data passed to callback
  }; // callback

#endif // HAVE_LIBPOPT

 no_mem:
  return(result);
} // dv_display_new

/* ----------------------------------------------------------------------------
 */
void
dv_display_show(dv_display_t *dv_dpy) {
  switch(dv_dpy->lib) {
  case e_dv_dpy_Xv:
#if HAVE_LIBXV
    dv_display_event(dv_dpy);
    XvShmPutImage(dv_dpy->dpy, dv_dpy->port,
		  dv_dpy->win, dv_dpy->gc,
		  dv_dpy->xv_image,
		  0, 0,					        /* sx, sy */
		  dv_dpy->width, dv_dpy->height,	        /* sw, sh */
		  0, 0,					        /* dx, dy */
		  dv_dpy->rwidth, dv_dpy->rheight,	        /* dw, dh */
		  True);
    XFlush(dv_dpy->dpy);
#endif // HAVE_LIBXV
    break;
  case e_dv_dpy_XShm:
    break;
  case e_dv_dpy_gtk:
#if HAVE_GTK
    gdk_draw_rgb_image(dv_dpy->image->window,
		       dv_dpy->image->style->fg_gc[dv_dpy->image->state],
		       0, 0, dv_dpy->width, dv_dpy->height,
		       GDK_RGB_DITHER_MAX, dv_dpy->pixels[0], dv_dpy->pitches[0]);
    gdk_flush();
    while(gtk_events_pending())
      gtk_main_iteration();
    gdk_flush();
#endif // HAVE_GTK
    break;
  case e_dv_dpy_SDL:
#if HAVE_SDL
    SDL_UnlockYUVOverlay(dv_dpy->overlay);
    SDL_DisplayYUVOverlay(dv_dpy->overlay, &dv_dpy->rect);
    SDL_LockYUVOverlay(dv_dpy->overlay);
#endif
    break;
  default:
    break;
  } // switch
} /* dv_display_show */

/* ----------------------------------------------------------------------------
 */
void
dv_display_exit(dv_display_t *dv_dpy) {
  if(!dv_dpy)
    return;

  switch(dv_dpy->lib) {
  case e_dv_dpy_Xv:
#if HAVE_LIBXV
    XvStopVideo(dv_dpy->dpy, dv_dpy->port, dv_dpy->win);
    if(dv_dpy->shminfo.shmaddr)
      shmdt(dv_dpy->shminfo.shmaddr);

    if(dv_dpy->shminfo.shmid > 0)
      shmctl(dv_dpy->shminfo.shmid, IPC_RMID, 0);

    if(dv_dpy->xv_image)
      free(dv_dpy->xv_image);
#endif // HAVE_LIBXV
    break;
  case e_dv_dpy_gtk:
    /* TODO: cleanup gtk and gdk stuff */
    break;
  case e_dv_dpy_XShm:
    break;
  case e_dv_dpy_SDL:
    /* TODO: SDL cleanup... */
    break;
  } // switch

  free(dv_dpy);
} /* dv_display_exit */

/* ----------------------------------------------------------------------------
 */
static gboolean
dv_display_gdk_init(dv_display_t *dv_dpy, gint *argc, gchar ***argv) {

#if HAVE_GTK
  dv_dpy->pixels[0] = calloc(1,dv_dpy->width * dv_dpy->height * 3);
  if(!dv_dpy) goto no_mem;
  gtk_init(argc, argv);
  gdk_rgb_init();
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());
  dv_dpy->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  dv_dpy->image = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(dv_dpy->window),dv_dpy->image);
  gtk_drawing_area_size(GTK_DRAWING_AREA(dv_dpy->image), 
			dv_dpy->width, dv_dpy->height);
  gtk_widget_set_usize(GTK_WIDGET(dv_dpy->image),
			dv_dpy->width, dv_dpy->height);
  gtk_widget_show(dv_dpy->image);
  gtk_widget_show(dv_dpy->window);
  gdk_flush();
  while(gtk_events_pending())
    gtk_main_iteration();
  gdk_flush();

  /* -------------------------------------------------------------------
	 * Setup converter functions for RGB video
	 */
  return TRUE;
 no_mem:
#endif // HAVE_GTK
  return FALSE;
} /* dv_display_gdk_init */

#if HAVE_LIBXV

static void
dv_display_event (dv_display_t *dv_dpy) {
  while (XCheckTypedWindowEvent (dv_dpy->dpy, dv_dpy->win,
				 ConfigureNotify, &dv_dpy->event)) {
    switch (dv_dpy->event.type) {
      case ConfigureNotify:
	dv_dpy->rwidth = dv_dpy->event.xconfigure.width;
	dv_dpy->rheight = dv_dpy->event.xconfigure.height;
	break;
      default:
	break;
    } // switch
  } // while
} /* dv_display_event */

#endif /* HAVE_LIBXV */

/* ----------------------------------------------------------------------------
 */
static gint
dv_display_Xv_init(dv_display_t *dv_dpy, gchar *w_name, gchar *i_name) {
  gint		ret = 0;
#if HAVE_LIBXV
  int		scn_id,
                ad_cnt, fmt_cnt,
                got_port, got_fmt,
                i, k;
  XGCValues	values;
  XSizeHints	hints;
  XWMHints	wmhints;
  XTextProperty	x_wname, x_iname;

  XvAdaptorInfo	*ad_info;
  XvImageFormatValues	*fmt_info;

  if(!(dv_dpy->dpy = XOpenDisplay(NULL))) {
    return 0;
  }

  dv_dpy->rwin = DefaultRootWindow(dv_dpy->dpy);
  scn_id = DefaultScreen(dv_dpy->dpy);

  /* -------------------------------------------------------------------
   * So let's first check for an available adaptor and port
   */
  if(Success == XvQueryAdaptors(dv_dpy->dpy, dv_dpy->rwin, &ad_cnt, &ad_info)) {
  
    for(i = 0, got_port = False; i < ad_cnt; ++i) {
      fprintf(stderr,
	      "Xv: %s: ports %ld - %ld\n",
	      ad_info[i].name,
	      ad_info[i].base_id,
	      ad_info[i].base_id +
	      ad_info[i].num_ports - 1);
      if (!(ad_info[i].type & XvImageMask)) {
	fprintf(stderr,
		"Xv: %s: XvImage NOT in capabilty list (%s%s%s%s%s )\n",
		ad_info[i].name,
		(ad_info[i].type & XvInputMask) ? " XvInput"  : "",
		(ad_info[i]. type & XvOutputMask) ? " XvOutput" : "",
		(ad_info[i]. type & XvVideoMask)  ?  " XvVideo"  : "",
		(ad_info[i]. type & XvStillMask)  ?  " XvStill"  : "",
		(ad_info[i]. type & XvImageMask)  ?  " XvImage"  : "");
	continue;
      }
      fmt_info = XvListImageFormats(dv_dpy->dpy, ad_info[i].base_id,&fmt_cnt);
      if (!fmt_info || fmt_cnt == 0) {
	fprintf(stderr, "Xv: %s: NO supported formats\n", ad_info[i].name);
	continue;
      }
      for(got_fmt = False, k = 0; k < fmt_cnt; ++k) {
	if (dv_dpy->format == fmt_info[k].id) {
	  got_fmt = True;
	  break;
	}
      }
      if (!got_fmt) {
	fprintf(stderr,
		"Xv: %s: format %#08x is NOT in format list ( ",
		ad_info[i].name,
                dv_dpy->format);
	for (k = 0; k < fmt_cnt; ++k) {
	  fprintf (stderr, "%#08x[%s] ", fmt_info[k].id, fmt_info[k].guid);
	}
	fprintf(stderr, ")\n");
	continue;
      }

      for(dv_dpy->port = ad_info[i].base_id, k = 0;
	  k < ad_info[i].num_ports;
	  ++k, ++(dv_dpy->port)) {
	if(!XvGrabPort(dv_dpy->dpy, dv_dpy->port, CurrentTime)) {
	  fprintf(stderr, "Xv: grabbed port %ld\n",
		  dv_dpy->port);
	  got_port = True;
	  break;
	}
      }
      if(got_port)
	break;
    }

  } else {
    // Xv extension probably not present
    return 0;
  }

  if(!ad_cnt) {
    fprintf(stderr, "Xv: (ERROR) no adaptor found!\n");
    return 0;
  }
  if(!got_port) {
    fprintf(stderr, "Xv: (ERROR) could not grab any port!\n");
    return 0;
  }

  dv_dpy->win = XCreateSimpleWindow(dv_dpy->dpy,
				       dv_dpy->rwin,
				       0, 0,
				       dv_dpy->width, dv_dpy->height,
				       0,
				       XWhitePixel(dv_dpy->dpy, scn_id),
				       XBlackPixel(dv_dpy->dpy, scn_id));
  hints.flags = PSize | PMaxSize | PMinSize;
  hints.min_width = dv_dpy->width / 16;
  hints.min_height = dv_dpy->height / 16;

  hints.max_width = dv_dpy->width * 16;
  hints.max_height = dv_dpy->height *16;

  wmhints.input = True;
  wmhints.flags = InputHint;

  XStringListToTextProperty(&w_name, 1 ,&x_wname);
  XStringListToTextProperty(&i_name, 1 ,&x_iname);


  XSetWMProperties(dv_dpy->dpy, dv_dpy->win,
		    &x_wname, &x_iname,
		    NULL, 0,
		    &hints, &wmhints, NULL);

  XSelectInput(dv_dpy->dpy, dv_dpy->win, ExposureMask | StructureNotifyMask);
  XMapRaised(dv_dpy->dpy, dv_dpy->win);
  XNextEvent(dv_dpy->dpy, &dv_dpy->event);

  dv_dpy->gc = XCreateGC(dv_dpy->dpy, dv_dpy->win, 0, &values);

  /* -------------------------------------------------------------------
   * Now we do shared memory allocation etc..
   */
  dv_dpy->xv_image = XvShmCreateImage(dv_dpy->dpy, dv_dpy->port,
					 dv_dpy->format, dv_dpy->pixels[0],
					 dv_dpy->width, dv_dpy->height,
					 &dv_dpy->shminfo);

  dv_dpy->shminfo.shmid = shmget(IPC_PRIVATE,
				     dv_dpy->len,
				     IPC_CREAT | 0777);

  dv_dpy->xv_image->data = dv_dpy->pixels[0] = dv_dpy->shminfo.shmaddr = 
    shmat(dv_dpy->shminfo.shmid, 0, 0);

  XShmAttach(dv_dpy->dpy, &dv_dpy->shminfo);
  XSync(dv_dpy->dpy, False);

  ret = 1;
#else
  fprintf(stderr, "libdv was compiled without Xv support\n");
#endif // HAVE_LIBXV
  return ret;
} /* dv_display_Xv_init */


#if HAVE_SDL

static void 
dv_center_window(SDL_Surface *screen)
{
    SDL_SysWMinfo info;

    SDL_VERSION(&info.version);
    if ( SDL_GetWMInfo(&info) > 0 ) {
        int x, y;
        int w, h;
        if ( info.subsystem == SDL_SYSWM_X11 ) {
            info.info.x11.lock_func();
            w = DisplayWidth(info.info.x11.display,
                             DefaultScreen(info.info.x11.display));
            h = DisplayHeight(info.info.x11.display,
                             DefaultScreen(info.info.x11.display));
            x = (w - screen->w)/2;
            y = (h - screen->h)/2;
            XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, x, y);
            info.info.x11.unlock_func();
        } // if
    } // if 
} // dv_center_window


static int
dv_display_SDL_init(dv_display_t *dv_dpy, gchar *w_name, gchar *i_name) {
  const SDL_VideoInfo *video_info;
  gint video_bpp;

  if(SDL_Init(SDL_INIT_VIDEO) < 0) goto no_sdl;
  /* Get the "native" video mode */
  video_info = SDL_GetVideoInfo();
  switch (video_info->vfmt->BitsPerPixel) {
  case 16:
  case 32:
    video_bpp = video_info->vfmt->BitsPerPixel;
    break;
  default:
    video_bpp = 16;
    break;
  } // switch 
  dv_dpy->sdl_screen = SDL_SetVideoMode(dv_dpy->width,dv_dpy->height,
					video_bpp,SDL_HWSURFACE);
  SDL_WM_SetCaption(w_name, i_name);
  dv_dpy->overlay = SDL_CreateYUVOverlay(dv_dpy->width, dv_dpy->height, dv_dpy->format,
					 dv_dpy->sdl_screen);
  if((!dv_dpy->overlay || (!dv_dpy->overlay->hw_overlay) ||  // we only want HW overlays
      SDL_LockYUVOverlay(dv_dpy->overlay)<0)) {
    goto no_overlay;
  } // if
  dv_center_window(dv_dpy->sdl_screen);
  dv_dpy->rect.x = 0;
  dv_dpy->rect.y = 0;
  dv_dpy->rect.w = dv_dpy->overlay->w;
  dv_dpy->rect.h = dv_dpy->overlay->h;
  dv_dpy->pixels[0] = dv_dpy->overlay->pixels[0];
  dv_dpy->pixels[1] = dv_dpy->overlay->pixels[1];
  dv_dpy->pixels[2] = dv_dpy->overlay->pixels[2];
  dv_dpy->pitches[0] = dv_dpy->overlay->pitches[0];
  dv_dpy->pitches[1] = dv_dpy->overlay->pitches[1];
  dv_dpy->pitches[2] = dv_dpy->overlay->pitches[2];
  return(True);

 no_overlay:
  if(dv_dpy->overlay) 
    SDL_FreeYUVOverlay(dv_dpy->overlay);
  SDL_Quit();
 no_sdl:
  return(False);

} /* dv_display_SDL_init */

#else

static int
dv_display_SDL_init(dv_display_t *dv_dpy, gchar *w_name, gchar *i_name) {
  fprintf(stderr,"playdv was compiled without SDL support\n");
  return(FALSE);
} /* dv_display_SDL_init */

#endif // HAVE_SDL

static void
dv_display_exit_handler(int code, void *arg)
{
  if(code && arg) {
    dv_display_exit(arg);
  } // if
} // dv_display_exit_handler


/* ----------------------------------------------------------------------------
 */
gboolean
dv_display_init(dv_display_t *dv_dpy, gint *argc, gchar ***argv, gint width, gint height, 
		dv_sample_t sampling, gchar *w_name, gchar *i_name) {

  dv_dpy->width = width;
  dv_dpy->height = height;
  
  switch(sampling) {
  case e_dv_sample_411:
  case e_dv_sample_422:
    dv_dpy->format = DV_FOURCC_YUY2;
    dv_dpy->len = dv_dpy->width * dv_dpy->height * 2;
    break;
  case e_dv_sample_420:
    dv_dpy->format = DV_FOURCC_YV12;
    dv_dpy->len = (dv_dpy->width * dv_dpy->height * 3) / 2;
    break;
  default:
    /* Not possible */
    break;
  } // switch

  switch(dv_dpy->arg_display) {
  case 0:
    /* Autoselect */
    /* Try to use Xv first, then SDL */
    if(dv_display_Xv_init(dv_dpy, w_name, i_name)) {
      goto Xv_ok;
    } else if(dv_display_SDL_init(dv_dpy, w_name, i_name)) {
      goto SDL_ok;
    } else {
      goto use_gtk;
    } // else
    break;
  case 1:
    /* Gtk */
    goto use_gtk;
    break;
  case 2:
    /* Xv */
    if(dv_display_Xv_init(dv_dpy, w_name, i_name)) {
      goto Xv_ok;
    } else {
      fprintf(stderr, "Attempt to display via Xv failed\n");
      goto fail;
    }
    break;
  case 3:
    /* SDL */
    if(dv_display_SDL_init(dv_dpy, w_name, i_name)) {
      goto SDL_ok;
    } else {
      fprintf(stderr, "Attempt to display via SDL failed\n");
      goto fail;
    }
    break;
  default:
    break;
  } // switch

 Xv_ok:
  fprintf(stderr, " Using Xv for display\n");
  dv_dpy->lib = e_dv_dpy_Xv;
  goto yuv_ok;

 SDL_ok:
  fprintf(stderr, " Using SDL for display\n");
  dv_dpy->lib = e_dv_dpy_SDL;
  goto yuv_ok;

 yuv_ok:

  dv_dpy->color_space = e_dv_color_yuv;

  switch(dv_dpy->format) {
  case DV_FOURCC_YUY2:
    dv_dpy->pitches[0] = width * 2;
    break;
  case DV_FOURCC_YV12:
    dv_dpy->pixels[1] = dv_dpy->pixels[0] + (width * height);
    dv_dpy->pixels[2] = dv_dpy->pixels[1] + (width * height / 4);
    dv_dpy->pitches[0] = width;
    dv_dpy->pitches[1] = width / 2;
    dv_dpy->pitches[2] = width / 2;
    break;
  } // switch

  goto ok;

 use_gtk:

  /* Try to use GDK since we couldn't get a HW YUV surface */
  dv_dpy->color_space = e_dv_color_rgb;
  dv_dpy->lib = e_dv_dpy_gtk;
  dv_dpy->len = dv_dpy->width * dv_dpy->height * 3;
  if(!dv_display_gdk_init(dv_dpy, argc, argv)) {
    fprintf(stderr,"Attempt to use gtk for display failed\n");
    goto fail;
  } // if 
  dv_dpy->pitches[0] = width * 3;
  fprintf(stderr, " Using gtk for display\n");

 ok:
  on_exit(dv_display_exit_handler, dv_dpy);
  return(TRUE);

 fail:
  fprintf(stderr, " Unable to establish a display method\n");
  return(FALSE);
} /* dv_display_init */
