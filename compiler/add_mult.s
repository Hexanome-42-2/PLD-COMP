.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    movl $6, %eax
    movl %eax, -4(%rbp)
    movl $5, %eax
    movl %eax, -8(%rbp)
    movl -4(%rbp), %eax
    movl %eax, -16(%rbp)
    movl -4(%rbp), %eax
    movl -16(%rbp), %edx
    imull %edx, %eax
    movl %eax, -20(%rbp)
    movl -8(%rbp), %eax
    movl %eax, -24(%rbp)
    movl -8(%rbp), %eax
    movl -24(%rbp), %edx
    imull %edx, %eax
    movl -20(%rbp), %edx
    addl %edx, %eax
    movl %eax, -28(%rbp)
    movl $1, %eax
    movl -28(%rbp), %edx
    addl %edx, %eax
    movl %eax, -12(%rbp)
    movl -12(%rbp), %eax
    popq %rbp
    ret 
