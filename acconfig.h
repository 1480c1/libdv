#ifndef DV_ACCONFIG_H
#define DV_ACCONFIG_H

/* 
   This file must contain an entry for every symbol which is
   AC_DEFINE'd in configure.in.   

   This is not an automatically generated file.

   This file must be maintained by hand to reflect
   the contents of configure.in.  

   This file is only a template file, the correct values of the
   individual symbols are set during generation of config.h by
   configure.

   See the autoconf manual for a complete picture of how this file
   fits into the build process.  Roughly, acconfig.h is one of the inputs
   to autoheader, which is used to generate config.h.in.  All of this
   happens during the bootstrap/autogen step.  
*/

/* Define as 1 if you have libglib */
#define HAVE_GLIB 0

/* Define as 1 if you have libgtk */
#define HAVE_GTK 0

/* Define as 1 if you have libsdl */
#define HAVE_SDL 0

/* Define as 1 if host is an IA32 */
#define ARCH_X86 0

/* See glibc documentation.  This gives us large file support (LFS) */
#define _GNU_SOURCE 0

/* Define to indicate DEBUGGING is enabled possibly with a level */
#define DEBUG 0

#endif // DV_ACCONFIG_H
