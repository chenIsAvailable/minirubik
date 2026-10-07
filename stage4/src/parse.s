parse:
    # init accumulators
    li   a3, 0              # perm = 0
    li   a4, 0              # orient = 0
    li   a5, 0              # sum = 0

    # step 1: permutation digits must be 1..7 with no repeats
    li   t0, 0              # i = 0
    li   t1, 0              # seen = 0
    li   t4, 7              # constant 7 (no calls here, safe outside loop)
parse_perm_chk:
    bgeu t0, t4, parse_perm_ok  # if i >= 7, all 7 digits checked
    add  t2, a0, t0         # t2 = &s[i]
    lbu  t3, 0(t2)          # t3 = s[i]
    addi t3, t3, -49        # c = s[i] - '1'
    bgeu t3, t4, parse_bad  # if c >= 7 (unsigned), not '1'..'7'
    li   t5, 1
    sll  t5, t5, t3         # mask = 1 << c
    and  t6, t1, t5         # t6 != 0 if digit c already seen
    bnez t6, parse_bad      # repeated digit
    or   t1, t1, t5         # mark digit c as seen
    addi t0, t0, 1          # i++
    j    parse_perm_chk

parse_perm_ok:
    # step 2: Lehmer rank, perm += fact[i] for each smaller digit to the right
    li   a6, 6              # constant 6 (outer loop limit)
    li   t0, 0              # i = 0
parse_i_loop:
    bgeu t0, a6, parse_rank_ok  # if i >= 6, rank done
    add  t2, a0, t0         # t2 = &s[i]
    lbu  t3, 0(t2)          # ci = s[i]
    la   t2, parse_fact     # t2 = base of parse_fact
    slli t5, t0, 1          # t5 = i*2 (each entry is 2 bytes)
    add  t2, t2, t5         # t2 = &parse_fact[i]
    lhu  t5, 0(t2)          # w = fact[i]
    addi t1, t0, 1          # j = i + 1
parse_j_loop:
    bgeu t1, t4, parse_i_next   # if j >= 7, next i
    add  t2, a0, t1         # t2 = &s[j]
    lbu  t6, 0(t2)          # cj = s[j]
    bgeu t6, t3, parse_j_next   # if cj >= ci, not smaller: skip
    add  a3, a3, t5         # perm += w
parse_j_next:
    addi t1, t1, 1          # j++
    j    parse_j_loop
parse_i_next:
    addi t0, t0, 1          # i++
    j    parse_i_loop

parse_rank_ok:
    # step 3: twists must be 1..3; orient = base-3 rank of first 6
    li   t0, 7              # i = 7
    li   t4, 14             # constant 14 (loop limit)
    li   a6, 13             # constant 13 (last twist index)
    li   t5, 3              # constant 3
parse_tw_loop:
    bgeu t0, t4, parse_tw_done  # if i >= 14, all twists read
    add  t2, a0, t0         # t2 = &s[i]
    lbu  t3, 0(t2)          # t3 = s[i]
    addi t3, t3, -49        # d = s[i] - '1'
    bgeu t3, t5, parse_bad  # if d >= 3 (unsigned), not '1'..'3'
    add  a5, a5, t3         # sum += d
    bgeu t0, a6, parse_tw_next  # i == 13: 7th twist only checked, not ranked
    slli t6, a4, 1          # t6 = orient*2
    add  a4, a4, t6         # orient = orient*3
    add  a4, a4, t3         # orient = orient*3 + d
parse_tw_next:
    addi t0, t0, 1          # i++
    j    parse_tw_loop
parse_tw_done:

    # step 4: sum % 3 must be 0, and the string must end at s[14]
parse_mod:
    bltu a5, t5, parse_mod_done # if sum < 3, done reducing
    addi a5, a5, -3         # sum -= 3
    j    parse_mod
parse_mod_done:
    bnez a5, parse_bad      # sum % 3 != 0: twists inconsistent
    lbu  t6, 14(a0)         # t6 = s[14]
    bnez t6, parse_bad      # string longer than 14 chars

    # success exit
    mv   a0, a3             # a0 = p
    mv   a1, a4             # a1 = o
    li   a2, 1              # a2 = 1: valid
    ret

parse_bad:
    li   a2, 0              # a2 = 0: invalid
    ret

.data
parse_fact: .half 720, 120, 24, 6, 2, 1
.text

