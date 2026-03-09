.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $160, %rsp
    movl $2, %eax
    movl %eax, -4(%rbp)
    movl $3, %eax
    movl %eax, -16(%rbp)
    movl -4(%rbp), %eax
    negl %eax
    movl -16(%rbp), %edx
    imull %edx, %eax
    movl %eax, -8(%rbp)
    movl -4(%rbp), %eax
    negl %eax
    movl %eax, -20(%rbp)
    movl $3, %eax
    movl %eax, -24(%rbp)
    movl -8(%rbp), %eax
    movl -24(%rbp), %edx
    addl %edx, %eax
    negl %eax
    movl -20(%rbp), %edx
    imull %edx, %eax
    movl %eax, -12(%rbp)
    movl -12(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret 
