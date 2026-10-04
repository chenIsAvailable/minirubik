.equ N, 100000000

.text
main:
    li t0, N             # t0 = N;
loop:
    addi t0, t0, -1      # t0 = t0 - 1;
    bne t0, x0, loop     # if (t0 != 0) goto loop;
    li a7, 10            # a7 = 10 (exit)
    ecall