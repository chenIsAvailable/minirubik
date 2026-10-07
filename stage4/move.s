move:
    # step 0: select tables by f (a2): 0 = R, 1 = B, 2 = D
    la   t0, perm_R         # assume f == 0 (R)
    la   t1, orient_R
    beqz a2, move_go        # f == 0: use R tables

    la   t0, perm_B         # otherwise assume f == 1 (B)
    la   t1, orient_B
    li   t3, 1
    beq  a2, t3, move_go    # f == 1: use B tables

    la   t0, perm_D         # otherwise f == 2 (D)
    la   t1, orient_D

move_go:
    # step 1: a0 = new p = perm[p]
    slli t2, a0, 1          # t2 = p*2 (each entry is 2 bytes)
    add  t2, t0, t2         # t2 = &perm[p]
    lhu  a0, 0(t2)          # a0 = new p = perm[p]

    # step 2: a1 = new o = orient[o]
    slli t2, a1, 1          # t2 = o*2 (each entry is 2 bytes)
    add  t2, t1, t2         # t2 = &orient[o]
    lhu  a1, 0(t2)          # a1 = new o = orient[o]

    ret                     # return new p in a0, new o in a1
