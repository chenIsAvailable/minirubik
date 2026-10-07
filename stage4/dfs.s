dfs:
    # prologue: reserve 48 bytes, save ra and s0-s8
    addi sp, sp, -48        # reserve 48 bytes on the stack
    sw   ra, 44(sp)         # save return address
    sw   s0, 40(sp)         # save s0
    sw   s1, 36(sp)         # save s1
    sw   s2, 32(sp)         # save s2
    sw   s3, 28(sp)         # save s3
    sw   s4, 24(sp)         # save s4
    sw   s5, 20(sp)         # save s5
    sw   s6, 16(sp)         # save s6
    sw   s7, 12(sp)         # save s7
    sw   s8, 8(sp)          # save s8

    # move arguments into saved registers
    mv   s0, a0             # s0 = p
    mv   s1, a1             # s1 = o
    mv   s2, a2             # s2 = g
    mv   s3, a3             # s3 = bound
    mv   s4, a4             # s4 = last

    # 2a: nodes++
    la   t0, nodes          # t0 = &nodes
    lw   t1, 0(t0)          # t1 = nodes
    addi t1, t1, 1          # t1 = nodes + 1
    sw   t1, 0(t0)          # nodes = nodes + 1

    # 2b: prune if g + h(p, o) > bound
    mv   a0, s0             # a0 = p
    mv   a1, s1             # a1 = o
    call h                  # a0 = h(p, o)
    add  t0, s2, a0         # t0 = g + h
    bltu s3, t0, dfs_no     # if bound < g + h, return 0

    # 2c: solved?
    or   t0, s0, s1         # t0 == 0 only if p == 0 and o == 0
    beqz t0, dfs_yes        # if solved, return 1

    # 3a: outer loop over faces f = 0..2, skip f == last
    li   s5, 0              # f = 0
dfs_f_loop:
    li   t0, 3
    bgeu s5, t0, dfs_no     # if f >= 3, all faces tried: return 0
    beq  s5, s4, dfs_f_next # if f == last, skip this face (continue)
    mv   s7, s0             # np = p (restart from original state)
    mv   s8, s1             # no = o

    # inner loop over turns t = 0..2 (turn 1/2/3 times)
    li   s6, 0              # t = 0
dfs_t_loop:
    li   t0, 3
    bgeu s6, t0, dfs_f_next # if t >= 3, go to next face

    # 3b: turn face f once more: (np, no) = move(np, no, f)
    mv   a0, s7             # a0 = np
    mv   a1, s8             # a1 = no
    mv   a2, s5             # a2 = f
    call move               # (a0, a1) = new state
    mv   s7, a0             # np = new p
    mv   s8, a1             # no = new o

    # 3c: record step g: move_face[g] = f, move_turn[g] = t
    la   t0, move_face      # t0 = base of move_face
    add  t0, t0, s2         # t0 = &move_face[g]
    sb   s5, 0(t0)          # move_face[g] = f
    la   t0, move_turn      # t0 = base of move_turn
    add  t0, t0, s2         # t0 = &move_turn[g]
    sb   s6, 0(t0)          # move_turn[g] = t

    # 3d: recurse: if (dfs(np, no, g + 1, bound, f)) return 1
    mv   a0, s7             # a0 = np
    mv   a1, s8             # a1 = no
    addi a2, s2, 1          # a2 = g + 1 (s2 itself stays g)
    mv   a3, s3             # a3 = bound
    mv   a4, s5             # a4 = f (becomes next level's last)
    call dfs                # a0 = 1 if a solution was found below
    bnez a0, dfs_yes        # found: return 1 all the way up

    addi s6, s6, 1          # t++
    j    dfs_t_loop         # back to the turn check

dfs_f_next:
    addi s5, s5, 1          # f++
    j    dfs_f_loop         # back to the face check

dfs_yes:
    li   a0, 1              # return 1: solution found
    j    dfs_ret
dfs_no:
    li   a0, 0              # return 0: no solution (fall through)
dfs_ret:
    # epilogue: restore ra, s0-s8, free 48 bytes, return
    lw   s8, 8(sp)          # restore s8
    lw   s7, 12(sp)         # restore s7
    lw   s6, 16(sp)         # restore s6
    lw   s5, 20(sp)         # restore s5
    lw   s4, 24(sp)         # restore s4
    lw   s3, 28(sp)         # restore s3
    lw   s2, 32(sp)         # restore s2
    lw   s1, 36(sp)         # restore s1
    lw   s0, 40(sp)         # restore s0
    lw   ra, 44(sp)         # restore return address
    addi sp, sp, 48         # give the 48 bytes back
    ret

