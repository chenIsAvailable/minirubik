.equ RENDER, 0

.text
main:
    li   a0, RENDER
    li   a1, 7
    li   a7, 10
    ecall