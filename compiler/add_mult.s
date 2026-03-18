.globl main
main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $160, %rsp
    jmp main_bloc_0
main_bloc_0:
    movl $5, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %edx
    movl %edx, %eax
foo_bloc_0:
    movl $6, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %edx
    movl %edx, %eax
main_exit:
    movq %rbp, %rsp
    popq %rbp
    ret 
