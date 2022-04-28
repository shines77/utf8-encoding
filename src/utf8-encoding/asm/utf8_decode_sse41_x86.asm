;*************************  utf8_decode_sse41_x86.asm  ************************************
; Author:           shines77
; Date created:     2022-04-28
; Last modified:    2022-04-28

;
; Position-independent code is generated if POSITIONINDEPENDENT is defined.
;
; CPU dispatching included for 386 and SSE4.1 instruction sets.
;
; Copyright (c) 2022 GNU General Public License www.gnu.org/licenses
;******************************************************************************
BITS 32

%define ALLOW_OVERRIDE 0                ; Set to one if override of standard function desired

global _utf8_decode_sse41               ; Function utf8_decode_sse41()

; Direct entries to CPU-specific versions
global _utf8_decode_Generic             ; Generic version for processors without SSE4.1
global _utf8_decode_SSE41               ; Version for processors with SSE4.1

; Imported from InstructionSet_x86.asm:
extern _InstructionSet                  ; Instruction set for CPU dispatcher

section .text

; utf8_decode() function

%if ALLOW_OVERRIDE
global ?OVR_utf8_decode
?OVR_utf8_decode:
%endif

_utf8_decode_sse4_1: ; function dispatching

%IFNDEF POSITIONINDEPENDENT
        jmp     near [utf8Dispatch] ; Go to appropriate version, depending on instruction set

%ELSE   ; Position-independent code

        call    get_thunk_edx          ; get reference point for position-independent code
RP:                                    ; reference point edx = offset RP

; Make the following instruction with address relative to RP:
        jmp     dword [edx+utf8Dispatch-RP]

%ENDIF

align 16
_utf8_decode_SSE41: ; SSE4.1 version
        push    ebx
        push    esi
        mov     esi, [esp+12]          ; haystack
        mov     eax, [esp+16]          ; needle
        movdqu  xmm1, [eax]            ; needle

align 8
haystacknext:
        ; [esi] = haystack
        pcmpistrm xmm1, [esi], 00001100b ; unsigned byte search, equal ordered, return mask in xmm0
        jc      matchbegin             ; found beginning of a match
        jz      nomatch                ; end of haystack found, no match
        add     esi, 16
        jmp     haystacknext

matchbegin:
        jz      foundshort             ; haystack ends here, a short match is found
        movd    eax, xmm0              ; bit mask of possible matches
nextindexbit:
        bsf     ecx, eax               ; index of first bit in mask of possible matches

        ; compare strings for full match
        lea     ebx, [esi+ecx]         ; haystack + index
        mov     edx, [esp+16]          ; needle

compareloop: ; compare loop for long match
        movdqu  xmm2, [edx]            ; paragraph of needle
        pcmpistrm xmm2, [ebx], 00001100B ; unsigned bytes, equal ordered, modifies xmm0
        ; (can't use "equal each, masked" because it inverts when past end of needle, but not when past end of both)

        jno     longmatchfail          ; difference found after extending partial match
        js      longmatchsuccess       ; end of needle found, and no difference
        add     edx, 16
        add     ebx, 16
        jmp     compareloop            ; loop to next 16 bytes

longmatchfail:
        ; remove index bit of first partial match
        btr     eax, ecx
        test    eax, eax
        jnz     nextindexbit           ; mask contains more index bits, loop to next bit in eax mask
        ; mask exhausted for possible matches, continue to next haystack paragraph
        add     esi, 16
        jmp     haystacknext           ; loop to next paragraph of haystack

longmatchsuccess: ; match found over more than one paragraph
        lea     eax, [esi+ecx]         ; haystack + index to begin of long match
        pop     esi
        pop     ebx
        ret

foundshort: ; match found within single paragraph
        movd    eax, xmm0              ; bit mask of matches
        bsf     eax, eax               ; index of first match
        add     eax, esi               ; pointer to first match
        pop     esi
        pop     ebx
        ret

nomatch: ; needle not found, return 0
        xor     eax, eax
        pop     esi
        pop     ebx
        ret

;_utf8_decode_SSE41: endp


align 16
_utf8_decode_Generic: ; generic version
        push    esi
        push    edi
        mov     esi, [esp+12]          ; haystack
        mov     edi, [esp+16]          ; needle

        mov     ax, [edi]
        test    al, al
        jz      _Found                 ; a zero-length needle is always found
        test    ah, ah
        jz      _SingleCharNeedle

_SearchLoop: ; search for first character match
        mov     cl, [esi]
        test    cl, cl
        jz      _NotFound              ; end of haystack reached without finding
        cmp     al, cl
        je      _FirstCharMatch        ; first character match
_IncompleteMatch:
        inc     esi
        jmp     _SearchLoop            ; loop through haystack

_FirstCharMatch:
        mov     ecx, esi               ; begin of match position
_MatchLoop:
        inc     ecx
        inc     edi
        mov     al, [edi]
        test    al, al
        jz      _Found                 ; end of needle. match ok
        cmp     al, [ecx]
        je      _MatchLoop
        ; match failed, recover and continue
        mov     edi, [esp+16]          ; needle
        mov     al, [edi]
        jmp     _IncompleteMatch

_NotFound: ; needle not found. return 0
        xor     eax, eax
        pop     edi
        pop     esi
        ret

_Found: ; needle found. return pointer to position in haystack
        mov     eax, esi
        pop     edi
        pop     esi
        ret

_SingleCharNeedle: ; Needle is a single character
        movzx   ecx, byte [esi]
        test    cl, cl
        jz      _NotFound				; end of haystack reached without finding
        cmp     al, cl
        je      _Found
        inc     esi
        jmp     _SingleCharNeedle		; loop through haystack


%IFDEF  POSITIONINDEPENDENT
get_thunk_edx: ; load caller address into edx for position-independent code
        mov edx, [esp]
        ret
%ENDIF

; CPU dispatching for strstr. This is executed only once
utf8CPUDispatch:
%IFNDEF POSITIONINDEPENDENT
        ; get supported instruction set
        call    _InstructionSet
        ; Point to generic version of strstr
        mov     ecx, _utf8_decode_Generic
        cmp     eax, 10                ; check SSE4.1
        jb      Q100
        ; SSE4.1 supported
        ; Point to SSE4.1 version of strstr
        mov     ecx, _utf8_decode_SSE41
Q100:   mov     [utf8Dispatch], ecx
        ; Continue in appropriate version of strstr
        jmp     ecx

%ELSE   ; Position-independent version
        ; get supported instruction set
        call    _InstructionSet
        call    get_thunk_edx
RP2:    ; reference point edx
        ; Point to generic version of strstr
        lea     ecx, [edx+_utf8_decode_Generic-RP2]
        cmp     eax, 10                ; check SSE4.1
        jb      Q100
        ; SSE4.1 supported
        ; Point to SSE4.1 version of strstr
        lea     ecx, [edx+_utf8_decode_SSE41-RP2]
Q100:   mov     [edx+utf8Dispatch-RP2], ecx
        ; Continue in appropriate version of strstr
        jmp     ecx
%ENDIF

SECTION .data

; Pointer to appropriate version. Initially points to dispatcher
utf8Dispatch DD utf8CPUDispatch
%IFDEF POSITIONINDEPENDENT
; Fix potential problem in Mac linker
        DD      0, 0
%ENDIF

SECTION .bss
; Append 16 bytes to end of last data section to allow reading past end of strings:
; (We might use names .bss$zzz etc. under Windows to make it is placed
; last, but the assembler gives sections with unknown names wrong attributes.
; Here, we are just relying on library data being placed after main data.
; This can be verified by making a link map file)
;        dq      0, 0
        resq 4
