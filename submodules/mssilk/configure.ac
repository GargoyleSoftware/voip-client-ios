#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.



AC_INIT([mssilk],[0.1])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([sdk/known_checksums/SILK_SDK_SRC_v1.0.8.zip.md5])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar --warnings=no-portability])



case $host in
        armv6-apple*)
        	SILK_FLAVOUR=ARM
                SILK_MAKE_OPTIONS="ADDED_DEFINES+=IPHONE TOOLCHAIN_PREFIX=XXXX"
        ;;
        armv7-apple*)
        	SILK_FLAVOUR=ARM
                SILK_MAKE_OPTIONS="USE_NEON=yes ADDED_DEFINES+=IPHONE TOOLCHAIN_PREFIX=XXXX"
        ;;
  *)
        SILK_FLAVOUR=FIX
     ;;
esac

AC_SUBST([SILK_FLAVOUR])
AC_SUBST([SILK_MAKE_OPTIONS])



# Checks for programs.
AC_PATH_PROG([WGET],[wget])
if test -z "$WGET"; then
   AC_MSG_ERROR(['wget' is required to build this package])
fi

AC_PATH_PROG([MD5SUM],[md5sum])
if test -z "$MD5SUM"; then
	AC_PATH_PROG([MD5SUM],[openssl])
	if test -n "$MD5SUM"; then
		MD5SUM="$MD5SUM md5"
	fi
fi

if test -z "$MD5SUM"; then
   AC_MSG_ERROR(['md5sum' is required to build this package])
fi

AC_PATH_PROG([UNZIP],[unzip])
if test -z "$UNZIP"; then
   AC_MSG_ERROR(['unzip' is required to build this package])
fi

AC_PATH_PROG([PATCH],[patch])
if test -z "$PATCH"; then
   AC_MSG_ERROR(['patch' is required to build this package])
fi


LT_INIT([disable-static])

# check for libraries that have pkg-config files installed
PKG_CHECK_MODULES(MEDIASTREAMER, mediastreamer >= 2.0.0)


# Checks for libraries.
AC_CHECK_LIB([m],[cosf])

case $target_os in 
	*mingw*)
		mingw_found=yes
		CFLAGS="$CFLAGS -fPIC"
	;;
esac

# Checks for library functions.

# Create the following files from their .in counterparts
AC_CONFIG_FILES([
  Makefile
  sdk/Makefile
])

#  src/Makefile
#  pkgconfig/Makefile
#  pkgconfig/silk.pc
#  man/Makefile
AC_OUTPUT
