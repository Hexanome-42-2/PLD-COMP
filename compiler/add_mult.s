.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $160, %rsp
    movl $4, %eax
    movl %eax, -4(%rbp)
    movl $5, %eax
    movl %eax, -8(%rbp)
    movl $6, %eax
    movl -8(%rbp), %edx
    addl %edx, %eax
    negl %eax
    movl -4(%rbp), %edx
    addl %edx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret 
