.equ RENDER, 0

.data
t16:    .half 1104, 1105
t8:     .byte 3, 4

.text
main:
    la   t0, t16
    lhu  a0, 2(t0)
    la   t1, t8
    lbu  a1, 1(t1)
.if RENDER
    li   a0, 0
.endif
    li   a7, 10
    ecall