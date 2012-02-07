/* Copyright (C) 2011 Belledonne Communications SARL
   File: speex.c

   Basic Speexdsp functions

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "modes.h"
#include <math.h>
#include "os_support.h"


#ifdef ARMV7NEON_ASM
EXPORT int libspeex_cpu_features=SPEEX_LIB_CPU_FEATURE_NEON;
#else
EXPORT int libspeex_cpu_features=0;
#endif


EXPORT int speex_lib_ctl(int request, void *ptr)
{
   switch (request)
   {
      case SPEEX_LIB_GET_MAJOR_VERSION:
         *((int*)ptr) = SPEEX_MAJOR_VERSION;
         break;
      case SPEEX_LIB_GET_MINOR_VERSION:
         *((int*)ptr) = SPEEX_MINOR_VERSION;
         break;
      case SPEEX_LIB_GET_MICRO_VERSION:
         *((int*)ptr) = SPEEX_MICRO_VERSION;
         break;
      case SPEEX_LIB_GET_EXTRA_VERSION:
         *((const char**)ptr) = SPEEX_EXTRA_VERSION;
         break;
      case SPEEX_LIB_GET_VERSION_STRING:
         *((const char**)ptr) = SPEEX_VERSION;
         break;
      case SPEEX_LIB_SET_CPU_FEATURES:
         libspeex_cpu_features=*(int*)ptr;
         break;
      /*case SPEEX_LIB_SET_ALLOC_FUNC:
         break;
      case SPEEX_LIB_GET_ALLOC_FUNC:
         break;
      case SPEEX_LIB_SET_FREE_FUNC:
         break;
      case SPEEX_LIB_GET_FREE_FUNC:
         break;*/
      default:
         speex_warning_int("Unknown wb_mode_query request: ", request);
         return -1;
   }
   return 0;
}

