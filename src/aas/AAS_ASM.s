@ Copyright (c) 2003-2021 James Daniels
@ Distributed under the MIT License
@ license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

.syntax unified
.text
.section    .text,"ax",%progbits
.ALIGN
.ARM

.GLOBAL AAS_DoDMA3

.GLOBAL AAS_MixAudio_SetMode_Normal
.GLOBAL AAS_MixAudio_SetMode_Boost
.GLOBAL AAS_MixAudio_SetMode_BoostAndClip

.GLOBAL AAS_MixAudio_SetMaxChans_2
.GLOBAL AAS_MixAudio_SetMaxChans_4
.GLOBAL AAS_MixAudio_SetMaxChans_8

.GLOBAL _AAS_vol_lookup


@ Volume lookup table. -1 means use multiply, 0 to 7 means use bit shift.

_AAS_vol_lookup:
	.byte 0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 6
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	.byte -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7

_ma_mul_r5_r0_r3:
	.word 0x00000010
	mul r5,r0,r3
_ma_mov_r5_r0_lsl_0:
	.word 0x00000001
	mov r5,r0,lsl #0
_ma_mlane_r5_r0_r3_r5:
	.word 0x00000011
	mlane r5,r0,r3,r5
_ma_add_r5_r5_r0_lsl_0:
	.word 0x00000011
	add r5,r5,r0,lsl #0


_ma_merge_noclip_start:
	@ ma_again:

	@ Merge
	ldr r14,[sp,#24]
	and r5,r14,r5,lsr #8
	and r6,r14,r6,lsr #8
	and r7,r14,r7,lsr #8
	and r8,r14,r8,lsr #8
	and r9,r14,r9,lsr #8
	and r10,r14,r10,lsr #8
	and r11,r14,r11,lsr #8
	and r12,r14,r12,lsr #8
	add r5,r5,r6,lsl #8
	add r6,r7,r8,lsl #8
	add r7,r9,r10,lsl #8
	add r8,r11,r12,lsl #8
	
	@ Store
	stmia r4!,{r5-r8}
	
	@ Loop
	subs r14,r14,#0x2000000
	.word 0xdaffffa5  @ ble ma_end
	
	@ ma_start:
	str r14,[sp,#24]
_ma_merge_noclip_end:


_ma_merge_boostnoclip_start:
	@ ma_again:

	@ Merge
	ldr r14,[sp,#24]
	and r5,r14,r5,lsr #7
	and r6,r14,r6,lsr #7
	and r7,r14,r7,lsr #7
	and r8,r14,r8,lsr #7
	and r9,r14,r9,lsr #7
	and r10,r14,r10,lsr #7
	and r11,r14,r11,lsr #7
	and r12,r14,r12,lsr #7
	add r5,r5,r6,lsl #8
	add r6,r7,r8,lsl #8
	add r7,r9,r10,lsl #8
	add r8,r11,r12,lsl #8
	
	@ Store
	stmia r4!,{r5-r8}
	
	@ Loop
	subs r14,r14,#0x2000000
	.word 0xdaffffa5  @ ble ma_end
	
	@ ma_start:
	str r14,[sp,#24]
_ma_merge_boostnoclip_end:


_ma_merge_clip_start:
	_ma_mask_0x80808080: .word 0x80808080

	.word 0,0 @ padding

	@ ma_again:
	@ Merge
	ldr r14,[sp,#24]
	.word 0xe51f2018  @ ldr r2,_ma_mask_0x80808080
	
	and r0,r14,r5,lsr #8
	and r1,r14,r6,lsr #8
	add r0,r0,r1,lsl #8
	and r5,r14,r5,lsr #7
	and r6,r14,r6,lsr #7
	add r5,r5,r6,lsl #8
	eor r0,r0,r5
	ands r0,r2,r0
	beq no_clip1  @ perhaps not worthwhile?
	and r1,r2,r5
	sub r1,r2,r1,lsr #7
	sub r0,r0,r0,lsr #7
	orr r0,r0,r0,lsl #1
	bic r5,r5,r0
	and r1,r1,r0
	orr r5,r5,r1
no_clip1:

	and r0,r14,r7,lsr #8
	and r1,r14,r8,lsr #8
	add r0,r0,r1,lsl #8
	and r7,r14,r7,lsr #7
	and r8,r14,r8,lsr #7
	add r6,r7,r8,lsl #8
	eor r0,r0,r6
	ands r0,r2,r0
	beq no_clip2  @ perhaps not worthwhile?
	and r1,r2,r6
	sub r1,r2,r1,lsr #7
	sub r0,r0,r0,lsr #7
	orr r0,r0,r0,lsl #1
	bic r6,r6,r0
	and r1,r1,r0
	orr r6,r6,r1
no_clip2:

	and r0,r14,r9,lsr #8
	and r1,r14,r10,lsr #8
	add r0,r0,r1,lsl #8
	and r9,r14,r9,lsr #7
	and r10,r14,r10,lsr #7
	add r7,r9,r10,lsl #8
	eor r0,r0,r7
	ands r0,r2,r0
	beq no_clip3  @ perhaps not worthwhile?
	and r1,r2,r7
	sub r1,r2,r1,lsr #7
	sub r0,r0,r0,lsr #7
	orr r0,r0,r0,lsl #1
	bic r7,r7,r0
	and r1,r1,r0
	orr r7,r7,r1
no_clip3:

	and r0,r14,r11,lsr #8
	and r1,r14,r12,lsr #8
	add r0,r0,r1,lsl #8
	and r11,r14,r11,lsr #7
	and r12,r14,r12,lsr #7
	add r8,r11,r12,lsl #8
	eor r0,r0,r8
	ands r0,r2,r0
	beq no_clip4  @ perhaps not worthwhile?
	and r1,r2,r8
	sub r1,r2,r1,lsr #7
	sub r0,r0,r0,lsr #7
	orr r0,r0,r0,lsl #1
	bic r8,r8,r0
	and r1,r1,r0
	orr r8,r8,r1
no_clip4:

	@ Store
	stmia r4!,{r5-r8}
	
	@ Loop
	subs r14,r14,#0x2000000
	.word 0xdaffff6d  @ ble ma_end
	
	@ ma_start:
	str r14,[sp,#24]
_ma_merge_clip_end:

_ma_clip:
add r12,pc,#0x540  @ adr r12,ma_buffer_start
.word 0xea000091  @ b ma_start
add r0,pc,#0x248  @ adr r0,ma_again

_ma_noclip:
add r12,pc,#0x460  @ adr r12,ma_buffer_start
.word 0xea000059  @ b ma_start
add r0,pc,#0x23c  @ adr r0,ma_again


AAS_MixAudio_SetMode_BoostAndClip:
	adr r12,_ma_clip
	adr r0,_ma_merge_clip_start
	movs r2, #(_ma_merge_clip_end-_ma_merge_clip_start)/4

do_mods:
	ldr r1,=_AAS_MixAudio_mod4
	add r2,r2,#0x84000000
	mov r3,#0x04000000
	add r3,r3,#0xd4
	stmia r3,{r0-r2}
	ldmia r12,{r1-r3}
	ldr r0,=_AAS_MixAudio_mod1
	str r1,[r0]
	ldr r0,=_AAS_MixAudio_mod2
	str r2,[r0]
	ldr r0,=_AAS_MixAudio_mod3
	str r3,[r0]
	bx lr


AAS_MixAudio_SetMode_Normal:
	adr r12,_ma_noclip
	adr r0,_ma_merge_noclip_start
	movs r2,#((_ma_merge_noclip_end-_ma_merge_noclip_start)/4)
	b do_mods
	
	
AAS_MixAudio_SetMode_Boost:
	adr r12,_ma_noclip
	adr r0,_ma_merge_boostnoclip_start
	movs r2,#((_ma_merge_boostnoclip_end-_ma_merge_boostnoclip_start)/4)
	b do_mods
	

_ma_2ch:
mov r6,#0x10000000
mov r10,#2
mov r14,#2
sub r1,r1,#(20*2)

_ma_4ch:
mov r6,#0x30000000
mov r10,#4
mov r14,#4
sub r1,r1,#(20*4)

_ma_8ch:
mov r6,#0x70000000
mov r10,#8
mov r14,#8
sub r1,r1,#(20*8)


AAS_MixAudio_SetMaxChans_4:
	adr r12,_ma_4ch

do_mods2:
	ldmia r12,{r0-r3}
	ldr r12,=_AAS_MixAudio_mod5
	str r0,[r12]
	ldr r12,=_AAS_MixAudio_mod6
	str r1,[r12]
	ldr r12,=_AAS_MixAudio_mod7
	str r2,[r12]
	ldr r12,=_AAS_MixAudio_mod8
	str r3,[r12]
	bx lr


AAS_MixAudio_SetMaxChans_8:
	adr r12,_ma_8ch
	b do_mods2
	
	
AAS_MixAudio_SetMaxChans_2:
	adr r12,_ma_2ch
	b do_mods2

.pool


AAS_DoDMA3:
	mov r3,#0x04000000
	add r3,r3,#0xd4
	stmia r3,{r0-r2}
	bx lr
