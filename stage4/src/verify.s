verify:
    # prologue: reserve 32 bytes, save ra and s0-s3
    addi sp, sp, -32        # reserve 32 bytes on the stack
    sw   ra, 28(sp)         # save return address
    sw   s0, 24(sp)         # save s0 (k)
    sw   s1, 20(sp)         # save s1 (len)
    sw   s2, 16(sp)         # save s2 (turns left)
    sw   s3, 12(sp)         # save s3 (face f)

    # step A: init and outer loop over k
    li   s0, 0              # k = 0
    mv   s1, a2             # s1 = len (a2 will be overwritten by move's f)
v_loop:
    bgeu s0, s1, v_done     # if k >= len, leave the loop

    # step B: read face and turn count of move k
    la   t0, move_face      # t0 = base of move_face
    add  t0, t0, s0         # t0 = &move_face[k]
    lbu  s3, 0(t0)          # s3 = f = move_face[k]

    la   t0, move_turn      # t0 = base of move_turn
    add  t0, t0, s0         # t0 = &move_turn[k]
    lbu  s2, 0(t0)          # s2 = move_turn[k] (0/1/2)
    addi s2, s2, 1          # s2 = number of move calls (1/2/3)

    # step C: call move s2 times (s2 >= 1)
v_turn:
    mv   a2, s3             # a2 = f (must reset before every call)
    call move               # (a0, a1) = move(a0, a1, f)
    addi s2, s2, -1         # one fewer turn left
    bnez s2, v_turn         # repeat while turns remain

    addi s0, s0, 1          # k++
    j    v_loop             # back to the check
v_done:
    # step D: a0 = (p == 0 && o == 0) ? 1 : 0
    or   t0, a0, a1         # t0 == 0 only if p == 0 and o == 0
    seqz a0, t0             # a0 = 1 if solved, else 0

    # epilogue: restore in reverse order, free stack, return
    lw   s3, 12(sp)         # restore s3
    lw   s2, 16(sp)         # restore s2
    lw   s1, 20(sp)         # restore s1
    lw   s0, 24(sp)         # restore s0
    lw   ra, 28(sp)         # restore return address
    addi sp, sp, 32         # give the 32 bytes back
    ret
