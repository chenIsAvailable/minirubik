# Test harness for verify. Not part of the solver.
# Build: copy /b stage4\verify_test.s + stage4\verify.s + stage4\move.s + stage4\tables.s stage4\build_verify_test.s
# Known solution for 21345671111111 (p=720, o=0), from stage 3 C:
#   R B' D2 R' B R' B' R D2 R B
# face: R=0 B=1 D=2   turn: X=0 X2=1 X'=2
.data
move_face: .byte 0, 1, 2, 0, 1, 0, 1, 0, 2, 0, 1, 0
move_turn: .byte 0, 2, 1, 2, 0, 2, 2, 0, 1, 0, 0, 0

.text
main:
    li   sp, 0x7ffffff0     # Ripes CLI starts with sp = 0, so set a stack

    # test 1: scrambled state + correct solution -> 1
    li   a0, 720
    li   a1, 0
    li   a2, 11
    call verify
    mv   s4, a0

    # test 2: solved state + same moves -> 0
    li   a0, 0
    li   a1, 0
    li   a2, 11
    call verify
    mv   s5, a0

    # test 3: solved state, 0 moves -> 1
    li   a0, 0
    li   a1, 0
    li   a2, 0
    call verify
    mv   s6, a0

    # test 4: scrambled state, 0 moves -> 0
    li   a0, 720
    li   a1, 0
    li   a2, 0
    call verify
    mv   s7, a0

    li   a7, 10
    ecall
    nop                 # Ripes executes one more instruction after exit ecall
    nop
