.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $1600, %rsp
    movl $2, %eax
    movl %eax, %edx
    movl $3, %eax
    imull %edx, %eax
    movl %eax, %edx
    movl $5, %eax
    movl %eax, %edx
    movl $8, %eax
    imull %edx, %eax
    addl %edx, %eax
    movl %eax, 0(%rbp)
    movl 0(%rbp), %eax
    popq %rbp
    ret 
