# Test harness for h. Not part of the solver.
# Build: copy /b stage4\h_test.s + stage4\h.s + stage4\tables.s stage4\build_h_test.s
# Expected: s1 = 0 (solved), s2 = 7 (21345671111111), s3 = 6 (12345672311111)
.text
main:
    li   a0, 0          # p for 12345671111111
    li   a1, 0          # o
    call h
    mv   s1, a0

    li   a0, 720        # p for 21345671111111
    li   a1, 0
    call h
    mv   s2, a0

    li   a0, 0          # p for 12345672311111
    li   a1, 405
    call h
    mv   s3, a0

    li   a7, 10
    ecall

    li   a7, 10
    ecall
    nop                 # Ripes executes one more instruction after exit ecall
    nop
