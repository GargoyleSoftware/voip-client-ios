/* Copyright (C) 2011 Belledonne Communications SARL
 */
/**
   @file resample_neon.h
   @brief Resampler functions (Neon version)
*/
/*
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

extern int libspeex_cpu_features;

spx_int32_t inner_product_single_neon(const spx_int16_t *a, const spx_int16_t *b, unsigned int len);
spx_int32_t inner_product_neon(const spx_int16_t *a, const spx_int16_t *b, unsigned int len);
spx_int32_t interpolate_product_single_neon(const spx_int16_t *a, const spx_int16_t *b, unsigned int len, const spx_uint32_t oversample, spx_int16_t *frac);

#define OVERRIDE_INNER_PRODUCT_SINGLE
static inline spx_int32_t inner_product_single(const spx_int16_t *a, const spx_int16_t *b, unsigned int len){
	spx_int32_t ret;

	if (!(libspeex_cpu_features & SPEEX_LIB_CPU_FEATURE_NEON)) {
		register int sum = 0;
		register int j;
		for(j=0;j<len;j++) sum += MULT16_16(a[j], b[j]);
		return sum;
	} else {
		return inner_product_single_neon(a, b, len);
	}
}

#define OVERRIDE_INNER_PROD
spx_int32_t inner_prod(const spx_int16_t *x, const spx_int16_t *y, int len){
	spx_int32_t ret;

	if (!(libspeex_cpu_features & SPEEX_LIB_CPU_FEATURE_NEON)) {
		spx_word32_t sum=0;
		len >>= 2;
		while(len--) { 
			spx_word32_t part=0;
			part = MAC16_16(part,*x++,*y++);
			part = MAC16_16(part,*x++,*y++);
			part = MAC16_16(part,*x++,*y++);
			part = MAC16_16(part,*x++,*y++);
			/* HINT: If you had a 40-bit accumulator, you could shift only at the end */
			sum = ADD32(sum,SHR32(part,6));
		}
		return sum;
	} else {
		return inner_product_neon(x, y, len);
	}
}

#define OVERRIDE_INTERPOLATE_PRODUCT_SINGLE
static inline spx_int32_t interpolate_product_single(const spx_int16_t *a, const spx_int16_t *b, unsigned int len, const spx_uint32_t oversample, spx_int16_t *frac){
if (!(libspeex_cpu_features & SPEEX_LIB_CPU_FEATURE_NEON)) {
		/*no neon*/
		/*from speex resampler.c*/
		int accum[4] = {0,0,0,0};
		int j;
		for(j=0;j<len;j++) {
			const short int curr_in=a[j];
			accum[0] += MULT16_16(curr_in,*(b  + j*oversample) );
			accum[1] += MULT16_16(curr_in,*((b + 1) + j*oversample));
			accum[2] += MULT16_16(curr_in,*((b + 2) + j*oversample));
			accum[3] += MULT16_16(curr_in,*((b + 3) + j*oversample));
		}
		return MULT16_32_Q15(frac[0],SHR32(accum[0], 1)) + MULT16_32_Q15(frac[1],SHR32(accum[1], 1)) + MULT16_32_Q15(frac[2],SHR32(accum[2], 1)) + MULT16_32_Q15(frac[3],SHR32(accum[3], 1));

	} else {
		return interpolate_product_single_neon(a, b, len, oversample, frac);
	}
}

