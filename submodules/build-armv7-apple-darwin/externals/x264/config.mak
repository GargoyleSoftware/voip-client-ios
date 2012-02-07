prefix=/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/armv7-apple-darwin
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=ARM
SYS=MACOSX
CC=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc
CFLAGS=-Wshadow -O3 -fno-fast-math  -Wall -I. -arch armv7 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk -falign-loops=16 -mdynamic-no-pic -std=gnu99 -fomit-frame-pointer -fno-tree-vectorize
DEPMM=-MM -g0
DEPMT=-MT
LD=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc -o 
LDFLAGS= -arch armv7 -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk -lm -lpthread
LIBX264=libx264.a
AR=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ar rc 
RANLIB=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ranlib
STRIP=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/strip
AS=extras/gas-preprocessor.pl /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc
ASFLAGS= -DPREFIX -DPIC  -Wall -I. -arch armv7 -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk -falign-loops=16 -mdynamic-no-pic -std=gnu99 -c -DBIT_DEPTH=8
EXE=
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
PROF_GEN_CC=-fprofile-generate
PROF_GEN_LD=-fprofile-generate
PROF_USE_CC=-fprofile-use
PROF_USE_LD=-fprofile-use
default: cli
install: install-cli
default: lib-static
install: install-lib-static
LDFLAGSCLI = 
CLI_LIBX264 = $(LIBX264)
