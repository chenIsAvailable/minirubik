h:
    slli t0, a1, 3          # step 1: t0 = row = o*8

    la   t2, twist_at8      # t2 = base of twist_at8
    add  t2, t2, t0         # t2 = &twist_at8[row]

    # step 2: t1 = pos0[p]
    la   t1, pos0           # t1 = base of pos0
    add  t1, t1, a0         # t1 = &pos0[p]
    lbu  t1, 0(t1)          # t1 = pos0[p]

    # step 3: tw0 = twist_at8[row + pos0[p]]
    add  t1, t2, t1         # t1 = &twist_at8[row + pos0[p]]
    lbu  t3, 0(t1)          # t3 = tw0

    # step 4: t1 = pos1[p]
    la   t1, pos1           # t1 = base of pos1
    add  t1, t1, a0         # t1 = &pos1[p]
    lbu  t1, 0(t1)          # t1 = pos1[p]

    # step 5: tw1 = twist_at8[row + pos1[p]]
    add  t1, t2, t1         # t1 = &twist_at8[row + pos1[p]]
    lbu  t4, 0(t1)          # t4 = tw1

    # step 6: t0 = p*9 = (p << 3) + p
    slli t0, a0, 3          # t0 = p*8
    add  t0, t0, a0         # t0 = p*8 + p = p*9

    # step 7: t1 = tw0*3 = (tw0 << 1) + tw0
    slli t1, t3, 1          # t1 = tw0*2
    add  t1, t1, t3         # t1 = tw0*2 + tw0 = tw0*3

    # step 8: t0 = k = p*9 + tw0*3 + tw1
    add  t0, t0, t1         # t0 = p*9 + tw0*3
    add  t0, t0, t4         # t0 = p*9 + tw0*3 + tw1 = k

    # step 9: t5 = a = pdb2[k]
    la   t2, pdb2           # t2 = base of pdb2
    add  t2, t2, t0         # t2 = &pdb2[k]
    lbu  t5, 0(t2)          # t5 = a = pdb2[k]

    # step 10: t6 = b = orient_dist[o]
    la   t2, orient_dist    # t2 = base of orient_dist
    add  t2, t2, a1         # t2 = &orient_dist[o]
    lbu  t6, 0(t2)          # t6 = b = orient_dist[o]

    # step 11: a0 = max(a, b), then return
    mv   a0, t5             # assume a is larger
    bgeu t5, t6, h_done     # if a >= b, keep a
    mv   a0, t6             # otherwise b is larger
h_done:
    ret                     # return to caller with a0
