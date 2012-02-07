#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([libilbc0-rfc3951],[0.6])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([downloads/known_checksums/rfc3951.txt.md5])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar --warnings=no-portability])

# Checks for programs.
AC_PROG_AWK
if test -z "$AWK"; then
   AC_MSG_ERROR([some form of 'awk' is required to build this package])
fi
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
   AC_MSG_ERROR(['md5sum is required to build this package])
fi

LT_INIT([disable-static])

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
  downloads/Makefile
  src/Makefile
  pkgconfig/Makefile
  pkgconfig/ilbc.pc
  man/Makefile
])
AC_OUTPUT
