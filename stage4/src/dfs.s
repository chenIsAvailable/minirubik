dfs:
    # iterative IDA* depth-first search for one bound (no recursion)
    # in:  a0 = p0, a1 = o0, a3 = bound (a2, a4 are ignored)
    # out: a0 = 1 if solved within bound, 0 otherwise
    # each level keeps its loop state in one 32-byte slot of dfs_stack:
    #   0: p   4: o   8: last   12: f   16: t   20: np   24: no
    # registers: s0 = p, s1 = o, s2 = d, s3 = bound, s4 = last,
    #            s5 = f, s6 = t, s7 = np, s8 = no, s9 = &dfs_stack[d]

    # prologue: runs once per bound, reserve 48 bytes, save ra and s0-s9
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
    sw   s9, 4(sp)          # save s9

    # root: nodes++ (no prune needed: bound >= h(p0, o0) always)
    la   t0, nodes          # t0 = &nodes
    lw   t1, 0(t0)          # t1 = nodes
    addi t1, t1, 1          # t1 = nodes + 1
    sw   t1, 0(t0)          # nodes = nodes + 1
    or   t0, a0, a1         # t0 == 0 only if p0 == 0 and o0 == 0
    beqz t0, dfs_found      # already solved: return 1

    # init
    mv   s0, a0             # s0 = p = p0
    mv   s1, a1             # s1 = o = o0
    li   s2, 0              # s2 = d = 0
    mv   s3, a3             # s3 = bound
    li   s4, 3              # s4 = last = 3 (no previous face)
    la   s9, dfs_stack      # s9 = &dfs_stack[0]

dfs_level:
    # a new level starts: f = 0
    li   s5, 0              # f = 0
dfs_f_loop:
    li   t0, 3
    bgeu s5, t0, dfs_back   # if f >= 3, this level is finished: backtrack
    beq  s5, s4, dfs_f_next # if f == last, skip this face
    mv   s7, s0             # np = p (restart from this level's state)
    mv   s8, s1             # no = o
    li   s6, 0              # t = 0
dfs_t_loop:
    li   t0, 3
    bgeu s6, t0, dfs_f_next # if t >= 3, go to next face

    # turn face f once more: (np, no) = move(np, no, f)
    mv   a0, s7             # a0 = np
    mv   a1, s8             # a1 = no
    mv   a2, s5             # a2 = f
    call move               # (a0, a1) = child state
    mv   s7, a0             # np = new p
    mv   s8, a1             # no = new o

    # nodes++ (every generated child counts, same as one dfs call before)
    la   t0, nodes          # t0 = &nodes
    lw   t1, 0(t0)          # t1 = nodes
    addi t1, t1, 1          # t1 = nodes + 1
    sw   t1, 0(t0)          # nodes = nodes + 1

    # inlined h(np, no): reads a0 (np), a1 (no); result in t5
    slli t0, a1, 3          # t0 = row = o*8
    la   t2, twist_at8      # t2 = base of twist_at8
    add  t2, t2, t0         # t2 = &twist_at8[row]

    la   t1, pos0           # t1 = base of pos0
    add  t1, t1, a0         # t1 = &pos0[p]
    lbu  t1, 0(t1)          # t1 = pos0[p]
    add  t1, t2, t1         # t1 = &twist_at8[row + pos0[p]]
    lbu  t3, 0(t1)          # t3 = tw0

    la   t1, pos1           # t1 = base of pos1
    add  t1, t1, a0         # t1 = &pos1[p]
    lbu  t1, 0(t1)          # t1 = pos1[p]
    add  t1, t2, t1         # t1 = &twist_at8[row + pos1[p]]
    lbu  t4, 0(t1)          # t4 = tw1

    slli t0, a0, 3          # t0 = p*8
    add  t0, t0, a0         # t0 = p*9
    slli t1, t3, 1          # t1 = tw0*2
    add  t1, t1, t3         # t1 = tw0*3
    add  t0, t0, t1         # t0 = p*9 + tw0*3
    add  t0, t0, t4         # t0 = k

    la   t2, pdb2           # t2 = base of pdb2
    add  t2, t2, t0         # t2 = &pdb2[k]
    lbu  t5, 0(t2)          # t5 = a = pdb2[k]

    la   t2, orient_dist    # t2 = base of orient_dist
    add  t2, t2, a1         # t2 = &orient_dist[o]
    lbu  t6, 0(t2)          # t6 = b = orient_dist[o]

    bgeu t5, t6, dfs_h_max  # if a >= b, t5 already holds max
    mv   t5, t6             # otherwise max = b
dfs_h_max:
    # prune if (d + 1) + h > bound
    add  t0, s2, t5         # t0 = d + h
    addi t0, t0, 1          # t0 = (d + 1) + h
    bltu s3, t0, dfs_t_next # pruned: do not descend, try next t

    # record step d: move_face[d] = f, move_turn[d] = t
    la   t0, move_face      # t0 = base of move_face
    add  t0, t0, s2         # t0 = &move_face[d]
    sb   s5, 0(t0)          # move_face[d] = f
    la   t0, move_turn      # t0 = base of move_turn
    add  t0, t0, s2         # t0 = &move_turn[d]
    sb   s6, 0(t0)          # move_turn[d] = t

    # child solved?
    or   t0, s7, s8         # t0 == 0 only if np == 0 and no == 0
    beqz t0, dfs_found      # solved: done

    # descend: save this level into dfs_stack[d], then go one level down
    sw   s0, 0(s9)          # slot.p    = p
    sw   s1, 4(s9)          # slot.o    = o
    sw   s4, 8(s9)          # slot.last = last
    sw   s5, 12(s9)         # slot.f    = f
    sw   s6, 16(s9)         # slot.t    = t
    sw   s7, 20(s9)         # slot.np   = np
    sw   s8, 24(s9)         # slot.no   = no
    addi s9, s9, 32         # s9 = &dfs_stack[d + 1]
    addi s2, s2, 1          # d = d + 1
    mv   s0, s7             # p = np (the child becomes this level's node)
    mv   s1, s8             # o = no
    mv   s4, s5             # last = f
    j    dfs_level          # start the new level

dfs_t_next:
    addi s6, s6, 1          # t++
    j    dfs_t_loop         # back to the turn check

dfs_f_next:
    addi s5, s5, 1          # f++
    j    dfs_f_loop         # back to the face check

dfs_back:
    # backtrack: this level is finished, return to the parent level
    beqz s2, dfs_notfound   # d == 0: the root is finished, not found
    addi s9, s9, -32        # s9 = &dfs_stack[d - 1]
    addi s2, s2, -1         # d = d - 1
    lw   s0, 0(s9)          # p    = slot.p
    lw   s1, 4(s9)          # o    = slot.o
    lw   s4, 8(s9)          # last = slot.last
    lw   s5, 12(s9)         # f    = slot.f
    lw   s6, 16(s9)         # t    = slot.t
    lw   s7, 20(s9)         # np   = slot.np
    lw   s8, 24(s9)         # no   = slot.no
    j    dfs_t_next         # continue with the next t at that level

dfs_found:
    li   a0, 1              # return 1: solution found
    j    dfs_ret
dfs_notfound:
    li   a0, 0              # return 0: no solution within bound
dfs_ret:
    # epilogue: restore ra, s0-s9, free 48 bytes, return
    lw   s9, 4(sp)          # restore s9
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

# fixed-size level stack: 12 slots x 32 bytes = 384 bytes
# the deepest saved slot is d = 9 (bound <= 11 and a descended child has h >= 1)
.data
dfs_stack:
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
.text
