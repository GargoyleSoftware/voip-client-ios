;*****************************************************************************
;* cpu-a.asm: x86 cpu utilities
;*****************************************************************************
;* Copyright (C) 2003-2011 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"

SECTION .text

;-----------------------------------------------------------------------------
; void cpu_cpuid( int op, int *eax, int *ebx, int *ecx, int *edx )
;-----------------------------------------------------------------------------
cglobal cpu_cpuid, 5,7
    push rbx
    push  r4
    push  r3
    push  r2
    push  r1
    mov  eax, r0d
    cpuid
    pop  rsi
    mov [rsi], eax
    pop  rsi
    mov [rsi], ebx
    pop  rsi
    mov [rsi], ecx
    pop  rsi
    mov [rsi], edx
    pop  rbx
    RET

;-----------------------------------------------------------------------------
; void cpu_xgetbv( int op, int *eax, int *edx )
;-----------------------------------------------------------------------------
cglobal cpu_xgetbv, 3,7
    push  r2
    push  r1
    mov  ecx, r0d
    xgetbv
    pop  rsi
    mov [rsi], eax
    pop  rsi
    mov [rsi], edx
    RET

%ifndef ARCH_X86_64

;-----------------------------------------------------------------------------
; int cpu_cpuid_test( void )
; return 0 if unsupported
;-----------------------------------------------------------------------------
cglobal cpu_cpuid_test
    pushfd
    push    ebx
    push    ebp
    push    esi
    push    edi
    pushfd
    pop     eax
    mov     ebx, eax
    xor     eax, 0x200000
    push    eax
    popfd
    pushfd
    pop     eax
    xor     eax, ebx
    pop     edi
    pop     esi
    pop     ebp
    pop     ebx
    popfd
    ret

;-----------------------------------------------------------------------------
; void stack_align( void (*func)(void*), void *arg );
;-----------------------------------------------------------------------------
cglobal stack_align
    push ebp
    mov  ebp, esp
    sub  esp, 12
    and  esp, ~15
    mov  ecx, [ebp+8]
    mov  edx, [ebp+12]
    mov  [esp], edx
    mov  edx, [ebp+16]
    mov  [esp+4], edx
    mov  edx, [ebp+20]
    mov  [esp+8], edx
    call ecx
    leave
    ret

%endif

;-----------------------------------------------------------------------------
; void cpu_emms( void )
;-----------------------------------------------------------------------------
cglobal cpu_emms
    emms
    ret

;-----------------------------------------------------------------------------
; void cpu_sfence( void )
;-----------------------------------------------------------------------------
cglobal cpu_sfence
    sfence
    ret

;-----------------------------------------------------------------------------
; void cpu_mask_misalign_sse( void )
;-----------------------------------------------------------------------------
cglobal cpu_mask_misalign_sse
    sub   rsp, 4
    stmxcsr [rsp]
    or dword [rsp], 1<<17
    ldmxcsr [rsp]
    add   rsp, 4
    ret
