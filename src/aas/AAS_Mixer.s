@ Copyright (c) 2003-2021 James Daniels
@ Distributed under the MIT License
@ license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

.TEXT
.SECTION    .iwram,"ax",%progbits
.ALIGN
.ARM

.GLOBAL AAS_MixAudio
.GLOBAL AAS_MixAudio_NoChange
.EXTERN AAS_DivTable

.GLOBAL _AAS_MixAudio_mod1
.GLOBAL _AAS_MixAudio_mod2
.GLOBAL _AAS_MixAudio_mod3
.GLOBAL _AAS_MixAudio_mod4
.GLOBAL _AAS_MixAudio_mod5
.GLOBAL _AAS_MixAudio_mod6
.GLOBAL _AAS_MixAudio_mod7
.GLOBAL _AAS_MixAudio_mod8

.pool

_ma_mov_r3_0: mov r3,#0
_ma_add_r0_r0_0: add r0,r0,#0

_ma_ldr_pc_0: .word 0xe51f0000+8  @ sort of equivalent to opcode(ldr r0,[pc,#-0])
_ma_mov_r14_r0_lsr_6: mov r14,r0,lsr #6
_ma_ldrsb_r0_r14_shifted: .word 0x3e1fe00d  @ (opcode("ldrsb r0,[r14,#+0]!")>>4) + (3<<28)
_ma_add_r0_r0_r0_lsl_16: adds r0,r0,r0,lsl #16  @ change regs as appropriate
_ma_vol_lookup_addr: .word _AAS_vol_lookup-1
_ma_total_iterations: .word 0x0
_ma_total_delta: .word 0x0
_ma_bytes_available: .word 0x0
_ma_no_skip: .word 0x0


	@ AAS_CODE_IN_IWRAM void AAS_MixAudio_NoChange( AAS_s8* mix_buffer, struct AAS_Channel chans[], int iterations );

AAS_MixAudio_NoChange:
	stmfd sp!,{r4-r11,r14}
	sub sp,sp,#16
	stmfd	sp!,{r0-r2}
	
	mov r9,#0
	str r9,_ma_no_skip
	ldr r10,_ma_bytes_available
	ldr r9,_ma_total_iterations
	cmp r9,r2
	movhi r9,r2
	mov r11,r9
	ldr r2,_ma_total_delta
	
	b ma_quickstart


	@ AAS_CODE_IN_IWRAM void AAS_MixAudio( AAS_s8* mix_buffer, struct AAS_Channel chans[], int iterations );

AAS_MixAudio:
	stmfd sp!,{r4-r11,r14}
	sub sp,sp,#16
	stmfd	sp!,{r0-r2}

	@ [sp] = _ma_mix_buffer
	@ [sp,#4] = _ma_chans
	@ [sp,#8] = _ma_to_go
	@ [sp,#12] = _ma_iterations_loop
	@ [sp,#16] = _ma_iterations_buffer
	@ [sp,#20] = _ma_iterations_scale_buffer
	@ [sp,#24] = _ma_loop_counter

	@ r0 = temp
	@ r1 = chans
	@ r2 = iterations in main loop 
	@ r3 = _ma_add_r0_r0_r0_lsl_16
	@ r4 = temp
	@ r5 = temp
	@ r6 = outer loop counter/active channels found/total delta>>2
	@ r7 = temp
	@ r8 = temp
	@ r9 = temp
	@ r10 = temp
	@ r11 = temp
	@ r12 = dest address
	@ r14 = temp
	
ma_do_setup:
_AAS_MixAudio_mod1:
	adr r12,ma_buffer_start
	ldr r3,_ma_add_r0_r0_r0_lsl_16
_AAS_MixAudio_mod5:
	mov r6,#0x70000000  @ was #0x30000000
	mov r2,#256
	
	
ma_setup_loop:
	ldrb r14,[r1],#20  @ effective_volume

	cmp r14,#0
	beq ma_skip  @ skip if effective_volume == 0
	
	
	@ Setup volume registers:
	@ r11 = increment for r4
	@ r14 = "mul r5,r0,r3"/"mov r5,r0,lsl #0"/"mlane r5,r0,r3,r5"/"add r5,r5,r0,lsl #0"
	@adr r10,_ma_vol_lookup-1
	ldr r10,_ma_vol_lookup_addr
	add r4,r10,#129  @ 129 = 1+_ma_mul_r5_r0_r3-_ma_vol_lookup
	ldrsb r10,[r10,r14]
	ands r5,r6,#0x0f000000  @ test if this is first active channel
	addne r4,r4,#16  @ 16 = _ma_mlane_r5_r0_r3_r5-_ma_mul_r5_r0_r3
	@adreq r4,_ma_mul_r5_r0_r3  @ use mul/mov if this is first non-zero chan
	@adrne r4,_ma_mlane_r5_r0_r3_r5  @ use mlane/add if this is first non-zero chan
	cmp r10,#0
	ldrlt r11,_ma_mov_r3_0  @ read "mov r3,#vol" if vol not power of 2
	addlt r11,r11,r14  @ set #vol in "mov r3,#vol" if vol not power of 2
	strlt r11,[r12],#4  @ write "mov r3,#vol" if vol not power of 2
	addge r4,r4,#8  @ increment if vol is power of 2
	ldmia r4,{r11,r14}  @ read mul/mlane/mov/add and increment
	addge r14,r14,r10,lsl #7  @ set lsl #val for mov/add if vol power of 2
	

	@ r0,r4,r5,r7,r8,r9,r10 available
	@ r3 = _ma_add_r0_r0_0
	@ r5 = delta/increment for r14
	@ r8 = _ma_divide_table
	@ r11 = temp (was increment for r14)
	@ r12 = dest address
	@ r14 = "mul r5,r0,r3"/"mov r5,r0,lsl #0"/"mlane r5,r0,r3,r5"/"add r5,r5,r0,lsl #0"
	
	@ Setup delta registers, write delta increment instructions:
	@ r5 = delta/increment for r14
	adr r7,_ma_chan_cache  @ could remove
	add r7,r7,r5,lsr #22  @ could remove
	ldr r10,_ma_ldr_pc_0  @ could pre-subtract _ma_chan_cache from _ma_ldr_pc_0 (would need to set at runtime)
	sub r10,r10,r7  @ could change to sub r10,r10,r5,lsr #22
	add r0,r10,r12
	eor r10,r10,#0x00100000  @ switch to str
	ldr r7,_ma_mov_r14_r0_lsr_6
	ldrh r5,[r1,#6-20]  @ delta
	and r9,r5,#0xff
	ldr r4,_ma_add_r0_r0_0
	add r9,r4,r9
	stmia r12!,{r0,r7,r9}
	add r7,r4,#0xc00
	add r7,r7,r5,lsr #8
	tst r7,#0xff
	strne r7,[r12],#4
	add r6,r6,r5,lsr #2
	add r7,r10,r12
	str r7,[r12],#4
	add r5,r11,r5,lsl #20
	
	
	@ Final setup:
	@ r0 = delta_pos
	@ r8 = x
	@ r9 = x_history
	@ r10 = local outer loop counter/_ma_ldrsb_r0_r14_shifted
	mov r0,#0x200  @ delta_pos = 0.5 (was 0)
	ldr r10,_ma_ldrsb_r0_r14_shifted
	mov r8,#0
	mov r9,#0
	
	
	@ r0 = delta_pos
	@ r1 = chans
	@ r2 = iterations in main loop
	@ r3 = _ma_add_r0_r0_r0_lsl_16
	@ r4 = temp
	@ r5 = delta/increment for r14
	@ r6 = outer loop counter/total delta<<1
	@ r7 = temp
	@ r8 = x
	@ r9 = x_history
	@ r10 = local outer loop counter/_ma_ldrsb_r0_r14_shifted
	@ r11 = temp (was increment for r14)
	@ r12 = dest address
	@ r14 = "mul r5,r0,r3"/"mov r5,r0,lsl #0"/"mlane r5,r0,r3,r5"/"add r5,r5,r0,lsl #0"

	@ Write instructions:
	mov r7,r10,lsl #4
	str r7,[r12],#4
	b ma_setup_inner_loop_first
	.word 0,0,0  @ padding
ma_setup_outer_loop:

	@ Write delta:
	add r0,r0,r5,lsr #20
	movs r4,r0,lsr #10
	beq ma_setup_inner_loop_skip1
	sub r0,r0,r4,lsl #10
	add r4,r4,r10,lsl #4
	mov r8,r7
	add r4,r4,r8,lsl #4
	str r4,[r12],#4
ma_setup_inner_loop_skip1:
	add r9,r8,r9,lsl #8

ma_setup_inner_loop_first:

	@ Write delta:
	add r0,r0,r5,lsr #20
	movs r4,r0,lsr #10
	beq ma_setup_inner_loop_skip2
	sub r0,r0,r4,lsl #10
	add r4,r4,r10,lsl #4
	subs r8,r8,#0x100
	movlt r8,#0x200
	add r4,r4,r8,lsl #4
	str r4,[r12],#4
ma_setup_inner_loop_skip2:
	add r9,r8,r9,lsl #8
	
	@ Write delta:
	add r0,r0,r5,lsr #20
	movs r4,r0,lsr #10
	beq ma_setup_inner_loop_skip3
	sub r0,r0,r4,lsl #10
	add r4,r4,r10,lsl #4
	subs r8,r8,#0x100
	movlt r8,#0x200
	add r4,r4,r8,lsl #4
	str r4,[r12],#4
ma_setup_inner_loop_skip3:
	add r9,r8,r9,lsl #8
	
	@ Write merge and mul/mla/mov/add:
	subs r11,r8,#0x100
	movlt r11,#0x200
	add r7,r14,r11,lsr #8
	add r4,r3,r11,lsl #4
	bic r11,r9,#0x00ff0000
	add r4,r4,r11,lsr #8
	stmia r12!,{r4,r7}
	add r14,r14,r5,lsl #12
	
	@ Write delta:
	add r0,r0,r5,lsr #20
	movs r4,r0,lsr #10
	beq ma_setup_inner_loop_skip4
	sub r0,r0,r4,lsl #10
	add r4,r4,r10,lsl #4
	subs r8,r8,#0x100
	movlt r8,#0x200
	add r4,r4,r8,lsl #4
	str r4,[r12],#4
ma_setup_inner_loop_skip4:
	add r9,r8,r9,lsl #8
	
	@ Write merge: (skips if unnecessary)
	subs r7,r8,#0x100
	movlt r7,#0x200
	bic r4,r9,#0x00ff0000
	cmp r4,r11
	addne r11,r3,r4,lsr #8
	addne r11,r11,r7,lsl #4
	strne r11,[r12],#4

	@ Write mul/mla/mov/add:
	add r4,r14,r7,lsr #8
	str r4,[r12],#4
	add r14,r14,r5,lsl #12
	
	subs r10,r10,#0x10000000
	bge ma_setup_outer_loop
	
	
	@ Calculate iterations until end of sample:
	ldr r10,[r1,#8-20]  @ pos
	ldr r0,[r1,#12-20]  @ end
	sub r0,r0,r10
	mov r5,r5,lsr #19
	ldr r8,_ma_divide_table
	ldrh r5,[r8,r5]
	mul r0,r5,r0
	cmp r2,r0,lsr #10
	movhi r2,r0,lsr #10
	
	add r6,r6,#0x01000000  @ increment active channels found
	
ma_skip:
	subs r6,r6,#0x10000000
	bge ma_setup_loop

	@ Write "b ma_again"
_AAS_MixAudio_mod3:
	adr r0,ma_again
	sub r0,r0,r12
	mov r0,r0,asr #2
	sub r0,r0,#2
	@bic r0,r0,#0xff000000
	@add r0,r0,#0xea000000
	adr r4,ma_buffer_end
	bic r0,r0,#0x15000000  @ offset always negative, so this is equivalent to above
	str r0,[r12],#4
	
	sub r10,r4,r12
	and r4,r6,#0x0f000000
	sub r10,r10,r4,lsr #21  @ r10 -= 8*used_channels
	@str r10,_ma_iterations_scale_buffer
	@str r10,[sp,#20]
	str r10,_ma_bytes_available
	
	ldr r11,[sp,#8]
	subs r9,r2,r11
	str r9,_ma_total_iterations
	@cmp r2,r11
	mov r9,#1
	movhi r9,#0
	str r9,_ma_no_skip
	movhi r2,r11
	
	mov r11,r2
	mov r9,r2
	bic r2,r6,#0xff000000
	str r2,_ma_total_delta
	
	@ r2 = total_delta>>2
	@ r9 = r11 = total_iterations - i.e. iterations until loop - argh! need to recalculate each call!
	@ r10 = ((bytes_available-(8*channels_used))<<4)
	
	@ Could remove need to recalc total_iterations each call by not doing "total_iterations = min( iterations, total_iterations )" - do min below instead and sub "iterations" from "total_iterations" afterwards. Problem: Need to accurately calculate "total_iterations" even when it is large. (Although only need to cope with total_iterations being twice as large as it is now because only ever re-use config once.)
	
ma_quickstart:
	@ldr r4,_ma_mix_buffer
	ldr r4,[sp]
	
	
	@ r1 = &_ma_mix_buffer
	@ r2 = total_delta>>2
	@ r3 = refill iterations<<3
	@ r4 = _ma_mix_buffer
	@ r5 = temp
	@ r6 = temp
	@ r7 = temp
	@ r8 = #0x04000000
	@ r9 = total iterations
	@ r10 = ((bytes_available-(8*channels_used))<<4)
	@ r11 = total iterations
	@ r12 = temp
	@ r14 = dest
	
	@ Calc iterations
	@ iterations = ((bytes_available-(8*channels_used))<<4)/(total_delta>>2)
	ldr r3,_ma_divide_table
ma_begin:  @ called from process loop
	mov r2,r2,lsl #1
	ldrh r3,[r3,r2]
	mul r3,r10,r3
	@str r3,_ma_iterations_buffer
	str r3,[sp,#16]
ma_begin2:
	cmp r9,r3,lsr #12  @ was asr #3
	movgt r9,r3,lsr #12  @ was asr #3

	sub r3,r11,r9
	@str r3,_ma_iterations_loop
	str r3,[sp,#12]

	cmp r9,#0
	ble ma_end


	@ r0 = temp
	@ r1 = temp
	@ r2 = 0xc0
	@ r3 = chans
	@ r4 = _ma_mix_buffer
	@ r5 = DMA address
	@ r6 = 2
	@ r7 = temp
	@ r8 = &_ma_chan_cache
	@ r9 = iterations
	@ r10 = loop counter
	@ r11 = temp
	@ r12 = buffer address
	@ r14 = temp

	@ Fill buffer
	adr r8,_ma_chan_cache
	mov r5,#0x04000000
	add r5,r5,#0xd4
	mov r6,#2
_AAS_MixAudio_mod6:
	mov r10,#8  @ was #4 - could perhaps set according to max # of channels / 2?
	mov r2,#0xc0
	adr r12,ma_buffer_end
	@ldr r3,_ma_chans
	ldr r3,[sp,#4]
	
ma_fill_buffer_loop:
	ldrb r14,[r3],#20  @ effective_volume
	cmp r14,#0
	beq ma_fill_buffer_skip
	ldr r7,[r3,#8-20]  @ pos
	bic r0,r7,#0x3
	ldrb r11,[r3,#3-20]  @ pos_fraction
	and r1,r2,r7,lsl #6
	add r1,r1,r11,lsr #2
	ldrh r14,[r3,#6-20]  @ delta
	mul r14,r9,r14
	add r11,r11,r14,lsl #2
	strb r11,[r3,#3-20]  @ pos_fraction
	add r7,r7,r11,lsr #8
	str r7,[r3,#8-20]  @ pos
	add r7,r6,r14,lsr #8  @ words to copy
	sub r12,r12,r7,lsl #2
	add r1,r1,r12,lsl #6 
	str r1,[r8],#4  @ IWRAM pos
	add r14,r7,#0x84000000
	stmia r5,{r0,r12,r14}
ma_fill_buffer_skip:
	subs r10,r10,#1
	bgt ma_fill_buffer_loop
	
	@ Setup registers for main loop
	@ldr r3,_ma_to_go
	ldr r3,[sp,#8]
	sub r3,r3,r9
	@str r3,_ma_to_go
	str r3,[sp,#8]
	
	@ Setup registers for main loop
	@mvn r14,#0xff00
	@add r14,r14,r9,lsl #24
	mvn r14,#0xff00
	bic r14,r14,#0x1000000
	add r14,r14,r9,lsl #25
_AAS_MixAudio_mod2:
	b ma_start

ma_end:
	@ldr r9,_ma_iterations_loop
	ldr r9,[sp,#12]
	movs r11,r9
	@ldrne r3,_ma_iterations_buffer
	ldrne r3,[sp,#16]
	bne ma_begin2

	@ldr r1,_ma_chans
	ldr r1,[sp,#4]
	@ should be after ble below?

	@ldr r9,_ma_to_go
	ldr r9,[sp,#8]
	cmp r9,#0
	ldrle r3,_ma_no_skip
	cmple r3,#0
	ble ma_chan_finished
	
	@ change so only branch if done all iterations and no samples have ended
	
	@ r0 = redo setup
	@ r1 = chans[] (was loop)
	@ r2 = total_delta>>2
	@ r3 = divide_table
	@ r4 = _ma_mix_buffer
	@ r5 = temp
	@ r6 = delta
	@ r7 = end
	@ r8 = unused
	@ r9 = loops to go
	@ r10 = _ma_specific_first
	@ r11 = &ma_chan0_start
	@ r12 = temp
	@ r13 = unused
	@ r14 = loop (was chans[])
	
	ldr r3,_ma_divide_table
_AAS_MixAudio_mod7:
	mov r14,#8  @ was #4
	mov r2,#0
	mov r0,#0

ma_check_chan_loop:
	ldrb r7,[r1],#20
	cmp r7,#0
	beq ma_chan_done
	
	ldrh r7,[r1,#6-20]  @ delta
	ldr r12,[r1,#8-20]  @ pos
	ldr r6,[r1,#12-20]  @ end
	sub r5,r6,r7,lsr #6
	sub r5,r5,#1
	cmp r5,r12
	ble ma_chan_do_loop
	
ma_chan_ok:
	add r2,r2,r7,lsr #2
	sub r5,r6,r12
	mov r7,r7,lsl #1
	ldrh r7,[r3,r7]
	mul r5,r7,r5
	cmp r9,r5,lsr #10
	movhi r9,r5,lsr #10
	
ma_chan_done:
	subs r14,r14,#1
	bgt ma_check_chan_loop
	
	cmp r0,#0
	bne ma_chan_has_finished
	
	movs r11,r9
	@ldrgt r10,_ma_iterations_scale_buffer
	@ldrgt r10,[sp,#20]
	ldrgt r10,_ma_bytes_available
	bgt ma_begin

ma_chan_finished:
	add sp,sp,#28
	ldmfd	sp!, {r4-r11, r14}
	bx lr @ Thumb interwork friendly.

ma_chan_has_finished:
	cmp r2,#0
	beq ma_chan_all_zeroes
	@ldr r2,_ma_to_go
	@ldr r2,[sp,#8]
_AAS_MixAudio_mod8:
	sub r1,r1,#(20*8)  @ was #(20*4)
	@str r4,_ma_mix_buffer
	str r4,[sp]
	b ma_do_setup

ma_chan_all_zeroes:  @ very rare - only happens if last active channel finished during this period
	@ldr r5,_ma_to_go
	ldr r5,[sp,#8]  @ can be 0 sometimes
	movs r5,r5,lsl #2
	add r5,r5,#0x85000000
	adr r2,_ma_empty
	mov r6,#0x04000000
	add r6,r6,#0xd4
	stmneia r6,{r2,r4,r5}  @ r5(_ma_to_go) can be 0 sometimes
	b ma_chan_finished
	


ma_chan_do_loop:
	ldr r5,[r1,#16-20]  @ loop_length
	cmp r5,#0
	
	mov r0,#1  @ redo setup    !!moved!!
	
	@ Loop sample
	subne r12,r12,r5
	strne r12,[r1,#8-20] @ pos
	bne ma_chan_ok

	@ Set inactive
	strh r5,[r1,#0-20]  @ effective_volume + active
	@mov r0,#1  @ redo setup    !!was here!!
	b ma_chan_done
	
	
_ma_chan_cache:
	.word 0,0,0,0,0,0,0,0  @ IWRAM pos, chan: 0,1,2,3,4,5,6,7

_ma_divide_table: .word AAS_DivTable

_ma_empty: .word 0
.word 0,0  @ padding

_AAS_MixAudio_mod4:

	@ r0-r2 : temp
	@ r3 : volume
	@ r4 : output address
	@ r5-r12 : output buffer
	@ r14 : sample address/loop/mask

ma_again:
	
	@ r0 = temp
	@ r1 = temp2
	@ r2 = mask_0x80808080
	@@ r3 free
	
	@ Make sure algo is as efficient as possible

	@ Merge
	@ldr r14,_ma_loop_counter
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
	ble ma_end

ma_start:
	@str r14,_ma_loop_counter
	str r14,[sp,#24]

ma_buffer_start: @ 2048 bytes  (1152 bytes would be equivalent to previous cache size)

	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  @ 128 bytes
	
ma_buffer_end:
