.globl main
main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $-4, %rsp
main_bloc:
    movl $6, %eax
    movl %eax, -4(%rbp)
    movl $3, %eax
    movl %eax, %edx
    movl -4(%rbp), %eax
    divl %edx
main_exit:
    movq %rbp, %rsp
    popq %rbp
    ret 
