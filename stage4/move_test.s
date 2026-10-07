# Test harness for move. Not part of the solver.
# Build: copy /b stage4\move_test.s + stage4\move.s + stage4\tables.s stage4\build_move_test.s
.text
main:
    # test 1: R from solved
    li   a0, 0
    li   a1, 0
    li   a2, 0
    call move
    mv   s1, a0
    mv   s2, a1

    # test 2: B from solved
    li   a0, 0
    li   a1, 0
    li   a2, 1
    call move
    mv   s3, a0
    mv   s4, a1

    # test 3: D from solved
    li   a0, 0
    li   a1, 0
    li   a2, 2
    call move
    mv   s5, a0
    mv   s6, a1

    # test 4: D from a non-solved state (p=720, o=405)
    li   a0, 720
    li   a1, 405
    li   a2, 2
    call move
    mv   s7, a0
    mv   s8, a1

    # test 5: R four times must return to solved
    li   a0, 0
    li   a1, 0
    li   a2, 0
    call move
    li   a2, 0
    call move
    li   a2, 0
    call move
    li   a2, 0
    call move
    mv   s9, a0
    mv   s10, a1

    li   a7, 10
    ecall
    nop                 # Ripes executes one more instruction after exit ecall
    nop
