.equ BYTES, 1048576       # 1 MiB

.text
main:
    li   t0, 0x10000000   # addr = 0x10000000;
    li   t1, BYTES        # t1 = BYTES;
    add  t1, t0, t1       # end = addr + BYTES;
    li   t2, -1           # value = -1;
loop:
    sw   t2, 0(t0)        # *(int *)addr = value;
    addi t0, t0, 4        # addr = addr + 4;
    bltu t0, t1, loop     # if (addr < end) goto loop;  (unsigned)
    li   a7, 10           # exit
    ecall