.section .text.saveregs

.global saveregs
saveregs:
    sts.l   pr, @-r15
    mov.l   r0, @-r15
    mov.l   r1, @-r15

    ! overwrite temp pr with IP start, to which we will return in restoreregs
    mov     r15, r0
    mov.l   p_ip, r1
    mov.l   r1, @(0x08,r0)

    mov.l   r2, @-r15
    mov.l   r3, @-r15
    mov.l   r4, @-r15
    mov.l   r5, @-r15
    mov.l   r6, @-r15
    mov.l   r7, @-r15
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    mov.l   r12, @-r15
    mov.l   r13, @-r15
    mov.l   r14, @-r15

    ! relocate to LoRAM and call main()
    mov.l   bios_hook_origin, r0
    mov.l   bios_hook_start, r1
    mov.l   bios_hook_end, r2

1:
    mov.l   @r0+, r4
    mov.l   r4, @r1
    add     #4, r1
    cmp/eq  r1, r2
    bf      1b


    mov.l   ram_origin, r0
    mov.l   ram_start, r1
    mov.l   ram_end, r2

1:
    mov.l   @r0+, r4
    mov.l   r4, @r1
    add     #4, r1
    cmp/eq  r1, r2
    bf      1b

    mov.l   p_restoreregs, r0
    lds     r0, pr

    mov.l   p_main, r0
    jmp     @r0
    nop

.align 4
__test_val:         .long 0xdeadbeef
p_restoreregs:      .long restoreregs
p_main:             .long _main
p_ip:               .long 0x06002e00

bios_hook_start:    .long __bios_hook_start
bios_hook_end:      .long __bios_hook_end
bios_hook_origin:   .long __bios_hook_origin

ram_start:          .long __ram_start
ram_end:            .long __ram_end
ram_origin:         .long __ram_origin


.section .text.restoreregs

.global restoreregs
restoreregs:
! TODO: set master and slave stack pointer?
! TODO: hide on the stack and clear RAM
    mov.l   @r15+, r14
    mov.l   @r15+, r13
    mov.l   @r15+, r12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    mov.l   @r15+, r8
    mov.l   @r15+, r7
    mov.l   @r15+, r6
    mov.l   @r15+, r5
    mov.l   @r15+, r4
    mov.l   @r15+, r3
    mov.l   @r15+, r2
    mov.l   @r15+, r1
    mov.l   @r15+, r0
    lds.l   @r15+, pr

    ldc     r0, gbr
    lds     r0, mach
    lds     r0, macl

    rts
    nop
