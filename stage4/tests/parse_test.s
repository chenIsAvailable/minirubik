# Test harness for parse. Not part of the solver.
# Build: copy /b stage4\parse_test.s + stage4\parse.s stage4\build_parse_test.s
# Strings are written as ASCII codes: '1' = 49 ... '7' = 55, then a 0 terminator.
.data
pt_s1: .byte 50,49,51,52,53,54,55,49,49,49,49,49,49,49,0,0  # 21345671111111  valid
pt_s2: .byte 49,50,51,52,53,54,55,50,51,49,49,49,49,49,0,0  # 12345672311111  valid
pt_s3: .byte 51,49,50,52,53,54,55,49,49,49,49,49,49,49,0,0  # 31245671111111  valid
pt_s4: .byte 50,49,51,52,53,54,55,49,49,49,49,49,49,50,0,0  # 21345671111112  bad twist sum
pt_s5: .byte 49,49,51,52,53,54,55,49,49,49,49,49,49,49,0,0  # 11345671111111  repeated digit
pt_s6: .byte 50,49,51,52,53,54,55,49,49,49,49,49,49,0,0,0   # 13 chars        too short
pt_s7: .byte 50,49,51,52,53,54,55,49,49,49,49,49,49,49,49,0 # 15 chars        too long

.text
main:
    la   a0, pt_s1
    call parse
    mv   s0, a0             # p
    mv   s1, a1             # o
    mv   s2, a2             # valid

    la   a0, pt_s2
    call parse
    mv   s3, a0
    mv   s4, a1
    mv   s5, a2

    la   a0, pt_s3
    call parse
    mv   s6, a0             # p
    mv   s7, a2             # valid

    la   a0, pt_s4
    call parse
    mv   s8, a2

    la   a0, pt_s5
    call parse
    mv   s9, a2

    la   a0, pt_s6
    call parse
    mv   s10, a2

    la   a0, pt_s7
    call parse
    mv   s11, a2

    li   a7, 10
    ecall
    nop                     # Ripes executes one more instruction after exit ecall
    nop

