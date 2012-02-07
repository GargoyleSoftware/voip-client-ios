prefix=/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build/..//../liblinphone-sdk/i386-apple-darwin
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=X86
SYS=MACOSX
CC=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/gcc
CFLAGS=-Wshadow -O3 -ffast-math  -Wall -I. -arch i386 -isysroot /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.0.sdk -falign-loops=16 -mdynamic-no-pic -march=i686 -mfpmath=sse -msse -std=gnu99 -fomit-frame-pointer -fno-tree-vectorize
DEPMM=-MM -g0
DEPMT=-MT
LD=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/gcc -o 
LDFLAGS= -arch i386 -isysroot /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.0.sdk -lm -lpthread
LIBX264=libx264.a
AR=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ar rc 
RANLIB=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ranlib
STRIP=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strip
AS=yasm
ASFLAGS= -O2 -f macho -DPREFIX -DBIT_DEPTH=8
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
