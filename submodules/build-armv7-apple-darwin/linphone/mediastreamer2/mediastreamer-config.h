/* mediastreamer-config.h.  Generated from mediastreamer-config.h.in by configure.  */
/* mediastreamer-config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Tells whether localisation is possible */
/* #undef ENABLE_NLS */

/* name of the gettext domain. Used in the call to 'bindtextdomain()' */
#define GETTEXT_PACKAGE "mediastreamer"

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
/* #undef HAVE_ALSA_ASOUNDLIB_H */

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
#define HAVE_CFLOCALECOPYCURRENT 1

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
#define HAVE_CFPREFERENCESCOPYAPPVALUE 1

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Defined if dlopen() is availlable */
#define HAVE_DLOPEN 1

/* Tells wheter localisation is possible */
/* #undef HAVE_GETTEXT */

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <kde/artsc/artsc.h> header file. */
/* #undef HAVE_KDE_ARTSC_ARTSC_H */

/* Define to 1 if you have the <libavcodec/avcodec.h> header file. */
#define HAVE_LIBAVCODEC_AVCODEC_H 1

/* Define to 1 if you have the <libswscale/swscale.h> header file. */
#define HAVE_LIBSWSCALE_SWSCALE_H 1

/* Defined if we have libv4l1 */
/* #undef HAVE_LIBV4L1 */

/* Defined if we have libv4l2 */
/* #undef HAVE_LIBV4L2 */

/* Define to 1 if you have the <linux/videodev2.h> header file. */
/* #undef HAVE_LINUX_VIDEODEV2_H */

/* Define to 1 if you have the <linux/videodev.h> header file. */
/* #undef HAVE_LINUX_VIDEODEV_H */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <portaudio.h> header file. */
/* #undef HAVE_PORTAUDIO_H */

/* Define to 1 if you have the <soundcard.h> header file. */
/* #undef HAVE_SOUNDCARD_H */

/* tells whether spandsp can be used */
/* #undef HAVE_SPANDSP */

/* have speexdsp library */
#define HAVE_SPEEXDSP 1

/* tells whether the noise arg of speex_echo_cancel can be used */
#define HAVE_SPEEX_NOISE 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/audio.h> header file. */
/* #undef HAVE_SYS_AUDIO_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <X11/extensions/Xvlib.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XVLIB_H */

/* Define to 1 if you have the <X11/extensions/Xv.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XV_H */

/* Define to 1 if you have the <X11/Xlib.h> header file. */
/* #undef HAVE_X11_XLIB_H */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* major version */
#define MEDIASTREAMER_MAJOR_VERSION 2

/* micro version */
#define MEDIASTREAMER_MICRO_VERSION 0

/* minor version */
#define MEDIASTREAMER_MINOR_VERSION 8

/* MEDIASTREAMER version number */
#define MEDIASTREAMER_VERSION "2.8.0"

/* Name of package */
#define PACKAGE "mediastreamer"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* path of data */
#define PACKAGE_DATA_DIR "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/armv7-apple-darwin/share"

/* Define to the full name of this package. */
#define PACKAGE_NAME "mediastreamer"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "mediastreamer 2.8.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "mediastreamer"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.8.0"

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "2.8.0"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* defined if alsa support is available */
/* #undef __ALSA_ENABLED__ */

/* defined if arts support is available */
/* #undef __ARTS_ENABLED__ */

/* defined if native macosx sound support is available */
/* #undef __MACSND_ENABLED__ */

/* defined if native macosx AQ sound support is available */
#define __MAC_AQ_ENABLED__ 1

/* defined if portaudio support is available */
/* #undef __PORTAUDIO_ENABLED__ */

/* Pulse audio support */
/* #undef __PULSEAUDIO_ENABLED__ */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
