# Test harness for search + dfs. Not part of the solver.
# Build: copy /b stage4\search_test.s + stage4\search.s + stage4\dfs.s + stage4\verify.s + stage4\move.s + stage4\h.s + stage4\tables.s stage4\build_search_test.s
# State 21345671111111 (p=720, o=0). Stage 3 C (solve_v0.c): 11 moves, 55232 nodes.
.data
nodes:     .word 0
move_face: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
move_turn: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

.text
main:
    li   sp, 0x7ffffff0     # Ripes CLI starts with sp = 0

    li   a0, 720            # p0
    li   a1, 0              # o0
    call search
    mv   s9, a0             # s9 = solution length

    la   t0, nodes
    lw   s10, 0(t0)         # s10 = total dfs calls

    li   a0, 720            # replay the solution from the same state
    li   a1, 0
    mv   a2, s9
    call verify
    mv   s11, a0            # s11 = 1 if the solution really solves it

    li   a7, 10
    ecall
    nop                     # Ripes executes one more instruction after exit ecall
    nop

