 .text
 .global elem
elem:
 addi sp ,sp, -16
.Lelem_entry:
 mv t0, a0
 addi t1, sp, 0
 sw t0, 0(t1)
 addi t0, sp, 0
 lw t0, 0(t0)
 sw t0, 4(sp)
 lw t0, 4(sp)
 li t1, 1
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 8(sp)
 lw t0, 8(sp)
 lw t0, 0(t0)
 sw t0, 12(sp)
 lw a0, 12(sp)
 addi sp ,sp, 16
 ret
 .global row
row:
 addi sp ,sp, -32
 sw ra, 28(sp)
.Lrow_entry:
 mv t0, a0
 addi t1, sp, 0
 sw t0, 0(t1)
 addi t0, sp, 0
 lw t0, 0(t0)
 sw t0, 4(sp)
 lw t0, 4(sp)
 li t1, 1
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 8(sp)
 lw t0, 8(sp)
 li t1, 0
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 12(sp)
 lw a0, 12(sp)
 call elem
 sw a0, 16(sp)
 lw a0, 16(sp)
 lw ra, 28(sp)
 addi sp ,sp, 32
 ret
 .global forward
forward:
 addi sp ,sp, -16
 sw ra, 12(sp)
.Lforward_entry:
 mv t0, a0
 addi t1, sp, 0
 sw t0, 0(t1)
 addi t0, sp, 0
 lw t0, 0(t0)
 sw t0, 4(sp)
 lw a0, 4(sp)
 call row
 sw a0, 8(sp)
 lw a0, 8(sp)
 lw ra, 12(sp)
 addi sp ,sp, 16
 ret
 .global main
main:
 addi sp ,sp, -112
 sw ra, 108(sp)
.Lmain_entry:
 addi t0, sp, 0
 li t1, 0
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 24(sp)
 lw t0, 24(sp)
 li t1, 0
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 28(sp)
 li t0, 1
 lw t1, 28(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 0
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 32(sp)
 lw t0, 32(sp)
 li t1, 1
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 36(sp)
 li t0, 2
 lw t1, 36(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 0
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 40(sp)
 lw t0, 40(sp)
 li t1, 2
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 44(sp)
 li t0, 3
 lw t1, 44(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 1
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 48(sp)
 lw t0, 48(sp)
 li t1, 0
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 52(sp)
 li t0, 4
 lw t1, 52(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 1
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 56(sp)
 lw t0, 56(sp)
 li t1, 1
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 60(sp)
 li t0, 5
 lw t1, 60(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 1
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 64(sp)
 lw t0, 64(sp)
 li t1, 2
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 68(sp)
 li t0, 6
 lw t1, 68(sp)
 sw t0, 0(t1)
 addi t0, sp, 0
 li t1, 0
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 72(sp)
 lw a0, 72(sp)
 call forward
 sw a0, 76(sp)
 addi t0, sp, 0
 li t1, 0
 li t2, 12
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 80(sp)
 lw t0, 80(sp)
 li t1, 0
 li t2, 4
 mul t1, t1, t2
 add t0, t0, t1
 sw t0, 84(sp)
 lw a0, 84(sp)
 call elem
 sw a0, 88(sp)
 lw t0, 76(sp)
 lw t1, 88(sp)
 add t0 , t0, t1
 sw t0, 92(sp)
 lw a0, 92(sp)
 lw ra, 108(sp)
 addi sp ,sp, 112
 ret
