.data
nodes:     .word 0
move_face: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
move_turn: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.text

main:
    li   sp, 0x7ffffff0     # Ripes CLI starts with sp = 0, so set a stack

    # parse the inlined state string
    la   a0, state          # a0 = address of state string
    call parse              # a0 = p, a1 = o, a2 = 1 if valid
    beqz a2, main_bad       # invalid input
    mv   s0, a0             # s0 = p (needed again by verify)
    mv   s1, a1             # s1 = o

    # search for the shortest solution
    call search             # a0 = len, or -1 if not found (a0, a1 = p, o already)
    mv   s2, a0             # s2 = len
    bltz s2, main_bad       # len < 0 (signed): not found

    # replay the solution and check it reaches (0, 0)
    mv   a0, s0             # a0 = p
    mv   a1, s1             # a1 = o
    mv   a2, s2             # a2 = len
    call verify             # a0 = 1 if solved
    mv   s3, a0             # s3 = verify result
    j    main_exit

main_bad:
    li   s2, -1             # no valid solution length
    li   s3, 0              # not verified

main_exit:
    li   a7, 10             # exit system call
    ecall
    nop                     # Ripes executes one more instruction after exit ecall
    nop

