.globl main
main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $0, %rsp
main_bloc:
    movl $5, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %edx
    movl %edx, %eax
main_exit:
    movq %rbp, %rsp
    popq %rbp
    ret 
