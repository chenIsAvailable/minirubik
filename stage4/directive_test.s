.equ RENDER, 0

.data
t16:    .half 1104, 1105
t8:     .byte 3, 4

.text
main:
    la   t0, t16
    lhu  a0, 2(t0)        # 讀 t16 的第 2 個 half,應該是 1105
    la   t1, t8
    lbu  a1, 1(t1)        # 讀 t8 的第 2 個 byte,應該是 4
.if RENDER
    li   a0, 0            # RENDER 是 0,這行不應該被組譯
.endif
    li   a7, 10           # 結束程式
    ecall