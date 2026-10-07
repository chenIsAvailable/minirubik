search:
    # prologue: reserve 16 bytes, save ra and s0-s2
    addi sp, sp, -16        # reserve 16 bytes on the stack
    sw   ra, 12(sp)         # save return address
    sw   s0, 8(sp)          # save s0 (p0)
    sw   s1, 4(sp)          # save s1 (o0)
    sw   s2, 0(sp)          # save s2 (bound)

    mv   s0, a0             # s0 = p0
    mv   s1, a1             # s1 = o0

    # bound = h(p0, o0)
    mv   a0, s0             # a0 = p0
    mv   a1, s1             # a1 = o0
    call h                  # a0 = h(p0, o0)
    mv   s2, a0             # bound = h(p0, o0)

srch_loop:
    li   t0, 12
    bgeu s2, t0, srch_fail  # if bound >= 12, give up

    # if (dfs(p0, o0, 0, bound, 3)) found
    mv   a0, s0             # a0 = p0
    mv   a1, s1             # a1 = o0
    li   a2, 0              # a2 = g = 0
    mv   a3, s2             # a3 = bound
    li   a4, 3              # a4 = last = 3 (no previous face)
    call dfs                # a0 = 1 if solved within bound
    bnez a0, srch_found     # found: return bound

    addi s2, s2, 1          # bound++
    j    srch_loop          # try again with a larger bound

srch_found:
    mv   a0, s2             # return bound (solution length)
    j    srch_ret
srch_fail:
    li   a0, -1             # return -1: not found
srch_ret:
    # epilogue: restore in reverse order, free stack, return
    lw   s2, 0(sp)          # restore s2
    lw   s1, 4(sp)          # restore s1
    lw   s0, 8(sp)          # restore s0
    lw   ra, 12(sp)         # restore return address
    addi sp, sp, 16         # give the 16 bytes back
    ret
