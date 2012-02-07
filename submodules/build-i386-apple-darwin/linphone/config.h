/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Tells whether localisation is possible */
/* #undef ENABLE_NLS */

/* Defined when using gsm at nonstandard rates */
/* #undef ENABLE_NONSTANDARD_GSM */

/* The name of the gettext package name */
#define GETTEXT_PACKAGE "linphone"

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

/* Defined when eXosip_get_socket is available */
/* #undef HAVE_EXOSIP_GET_SOCKET */

/* Defined when eXosip_get_version is available */
#define HAVE_EXOSIP_GET_VERSION 1

/* Defined when eXosip_tls_verify_certificate is available */
#define HAVE_EXOSIP_TLS_VERIFY_CERTIFICATE 1

/* If present, the getenv function allows fim to read environment variables.
   */
#define HAVE_GETENV 1

/* Define to 1 if you have the `getifaddrs' function. */
#define HAVE_GETIFADDRS 1

/* Tells wheter localisation is possible */
/* #undef HAVE_GETTEXT */

/* Define to 1 if you have the `get_current_dir_name' function. */
/* #undef HAVE_GET_CURRENT_DIR_NAME */

/* Defined when gtk osx is used */
/* #undef HAVE_GTK_OSX */

/* Define to 1 if you have the <history.h> header file. */
/* #undef HAVE_HISTORY_H */

/* Define if you have the iconv() function. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `eXosip2' library (-leXosip2). */
#define HAVE_LIBEXOSIP2 1

/* Define to 1 if you have the `osip2' library (-losip2). */
/* #undef HAVE_LIBOSIP2 */

/* Define to 1 if you have the `osipparser2' library (-losipparser2). */
/* #undef HAVE_LIBOSIPPARSER2 */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* NOTIFY1 support */
/* #undef HAVE_NOTIFY1 */

/* NOTIFY4 support */
/* #undef HAVE_NOTIFY4 */

/* Define to 1 if you have the <readline.h> header file. */
/* #undef HAVE_READLINE_H */

/* Define to 1 if you have the <readline/history.h> header file. */
/* #undef HAVE_READLINE_HISTORY_H */

/* Define to 1 if you have the <readline/readline.h> header file. */
/* #undef HAVE_READLINE_READLINE_H */

/* Define if sighandler_t available */
/* #undef HAVE_SIGHANDLER_T */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `stpcpy' function. */
#define HAVE_STPCPY 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strndup' function. */
#define HAVE_STRNDUP 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <X11/Xlib.h> header file. */
/* #undef HAVE_X11_XLIB_H */

/* All supported languages */
#define LINPHONE_ALL_LANGS "fr it de ja es pl cs nl sv pt_BR hu ru zh_CN nb_NO zh_TW"

/* Windows appdata subdir where linphonerc can be found */
#define LINPHONE_CONFIG_DIR "Linphone"

/* path of liblinphone plugins, not mediastreamer2 plugins */
#define LINPHONE_PLUGINS_DIR "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/i386-apple-darwin/lib/liblinphone/plugins"

/* Linphone's version number */
#define LINPHONE_VERSION "3.5.0"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "linphone"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "linphone-developers@nongnu.org"

/* Defines the place where data are found */
#define PACKAGE_DATA_DIR "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/i386-apple-darwin/share"

/* Defines the place where locales can be found */
#define PACKAGE_LOCALE_DIR "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/i386-apple-darwin/share/locale"

/* Define to the full name of this package. */
#define PACKAGE_NAME "linphone"

/* Defines the place where linphone sounds are found */
#define PACKAGE_SOUND_DIR "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/i386-apple-darwin/share/sounds/linphone"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "linphone 3.5.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "linphone"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.5.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Tell whether date_version.h must be used */
/* #undef USE_BUILDDATE_VERSION */

/* Version number of package */
#define VERSION "3.5.0"

/* defined if video support is available */
#define VIDEO_ENABLED 1

/* Tell whether RSVP support should be compiled. */
/* #undef VINCENT_MAURY_RSVP */

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

/* Defined if we are compiling for arm processor */
/* #undef __ARM__ */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
