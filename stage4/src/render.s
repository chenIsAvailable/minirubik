.text
    .globl led_turn

# void led_turn(int f)   a0 = f (0 = R, 1 = B, 2 = D)
# leaf function: no calls, no stack frame, uses only a and t registers
led_turn:
    slli  t0, a0, 3            # t0 = f * 8 (8 bytes per row)
    la    t1, led_src
    add   t1, t1, t0           # t1 = &src[f][0]
    la    t2, led_tw
    add   t2, t2, t0           # t2 = &tw[f][0]
    la    a1, led_p            # a1 = p
    la    a2, led_o            # a2 = o
    la    a3, led_tp           # a3 = tp (temporary)
    la    a4, led_to           # a4 = to (temporary)
    li    t4, 7                # loop limit
    li    a5, 3                # constant for mod 3

    li    t3, 0                # i = 0
led_turn_loop1:
    add   t5, t1, t3
    lbu   t5, 0(t5)            # t5 = from = src[f][i]

    add   t6, a1, t5
    lbu   t6, 0(t6)            # t6 = p[from]
    add   t0, a3, t3
    sb    t6, 0(t0)            # tp[i] = p[from]

    add   t6, a2, t5
    lbu   t6, 0(t6)            # t6 = o[from]
    add   t0, t2, t3
    lbu   t0, 0(t0)            # t0 = tw[f][i]
    add   t6, t6, t0           # t6 = o[from] + tw[f][i] (0..4)
    blt   t6, a5, led_turn_nomod
    addi  t6, t6, -3           # >= 3: subtract 3 once is enough
led_turn_nomod:
    add   t0, a4, t3
    sb    t6, 0(t0)            # to[i] = result

    addi  t3, t3, 1
    blt   t3, t4, led_turn_loop1

    li    t3, 0                # i = 0, copy back
led_turn_loop2:
    add   t5, a3, t3
    lbu   t6, 0(t5)            # tp[i]
    add   t5, a1, t3
    sb    t6, 0(t5)            # p[i] = tp[i]
    add   t5, a4, t3
    lbu   t6, 0(t5)            # to[i]
    add   t5, a2, t3
    sb    t6, 0(t5)            # o[i] = to[i]
    addi  t3, t3, 1
    blt   t3, t4, led_turn_loop2

    ret

    .globl led_draw

# void led_draw(void)
# leaf function: no calls, no stack frame, uses only a and t registers
led_draw:
    la    a1, led_pos          # a1 -> led_pos[i] (+1 each facelet)
    la    a2, led_slot         # a2 -> led_slot[i] (+1 each facelet)
    la    a3, led_off          # a3 -> led_off[i] (.half, +2 each facelet)
    addi  a4, a1, 24           # a4 = end of led_pos, loop stop condition
    li    a5, LED_MATRIX_0_WIDTH
    slli  a5, a5, 2            # a5 = bytes per LED row (width * 4)

led_draw_loop:
    lbu   t0, 0(a1)            # t0 = pos
    lbu   t1, 0(a2)            # t1 = slot

    la    t2, led_p
    add   t2, t2, t0
    lbu   t2, 0(t2)            # t2 = c = led_p[pos]
    la    t3, led_o
    add   t3, t3, t0
    lbu   t3, 0(t3)            # t3 = o = led_o[pos]

    add   t1, t1, t3           # j = slot + o (0..4)
    li    t4, 3
    blt   t1, t4, led_draw_nomod
    addi  t1, t1, -3           # j >= 3: subtract 3
led_draw_nomod:
    slli  t2, t2, 2            # c * 4
    add   t2, t2, t1           # c * 4 + j
    la    t3, led_home
    add   t3, t3, t2
    lbu   t3, 0(t3)            # t3 = face

    slli  t3, t3, 2            # face * 4 (.word)
    la    t4, led_rgb
    add   t4, t4, t3
    lw    t5, 0(t4)            # t5 = rgb

    lhu   t0, 0(a3)            # t0 = led_off[i]
    li    t6, LED_MATRIX_0_BASE
    add   t6, t6, t0           # t6 = address of this facelet's top-left LED

    li    t1, 3                # 3 rows
led_draw_row:
    sw    t5, 0(t6)            # 4 LEDs per row, unrolled
    sw    t5, 4(t6)
    sw    t5, 8(t6)
    sw    t5, 12(t6)
    add   t6, t6, a5           # next row
    addi  t1, t1, -1
    bnez  t1, led_draw_row

    addi  a1, a1, 1
    addi  a2, a2, 1
    addi  a3, a3, 2
    blt   a1, a4, led_draw_loop

    ret

    .globl render

# void render(int len)   a0 = len (number of moves in the solution)
# non-leaf: calls led_draw / led_turn, so it saves ra and the s registers it uses
render:
    addi  sp, sp, -32          # the RISC-V ABI keeps sp 16-byte aligned
    sw    ra, 28(sp)
    sw    s0, 24(sp)
    sw    s1, 20(sp)
    sw    s2, 16(sp)
    sw    s3, 12(sp)

    mv    s0, a0               # s0 = len (must survive the calls)

    # ---- 1. build the cube from the input string (no calls, t registers are enough) ----
    la    t0, state
    la    t1, led_p
    la    t2, led_o
    li    t3, 0                # i = 0
    li    t4, 7
render_init:
    add   t5, t0, t3           # t5 = &state[i]
    lbu   t6, 0(t5)            # state[i]
    addi  t6, t6, -49          # - '1' (ASCII 49)
    add   a1, t1, t3
    sb    t6, 0(a1)            # led_p[i]

    lbu   t6, 7(t5)            # state[7 + i]: the offset is in the instruction
    addi  t6, t6, -49
    add   a1, t2, t3
    sb    t6, 0(a1)            # led_o[i]

    addi  t3, t3, 1
    blt   t3, t4, render_init

    # ---- 2. draw the scrambled cube ----
    jal   ra, led_draw

    # ---- 3. replay the solution ----
    li    s1, 0                # k = 0
render_move:
    bge   s1, s0, render_done  # k >= len: done (also correct for len = 0)

    la    t0, move_face
    add   t0, t0, s1
    lbu   s3, 0(t0)            # s3 = face = move_face[k]
    la    t0, move_turn
    add   t0, t0, s1
    lbu   s2, 0(t0)
    addi  s2, s2, 1            # s2 = number of quarter turns = move_turn[k] + 1

render_rep:
    mv    a0, s3               # argument f
    jal   ra, led_turn
    addi  s2, s2, -1
    bnez  s2, render_rep

    jal   ra, led_draw         # redraw after every move
    addi  s1, s1, 1
    j     render_move

render_done:
    lw    s3, 12(sp)
    lw    s2, 16(sp)
    lw    s1, 20(sp)
    lw    s0, 24(sp)
    lw    ra, 28(sp)
    addi  sp, sp, 32
    ret

