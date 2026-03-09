.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    movl $2, %eax
    movl %eax, -4(%rbp)
    movl $3, %eax
    movl -4(%rbp), %edx
    imull %edx, %eax
    movl %eax, -8(%rbp)
    movl $5, %eax
    movl %eax, -12(%rbp)
    movl $8, %eax
    movl -12(%rbp), %edx
    imull %edx, %eax
    movl -8(%rbp), %edx
    addl %edx, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    popq %rbp
    ret 
