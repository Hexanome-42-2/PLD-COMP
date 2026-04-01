.globl main
main: 
    pushq %rbp
    movq %rsp, %rbp
    subq $160, %rsp
main_bloc_0:
    movl $65, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %edx
    movl %edx, %edi
    call putchar@PLT
    movl $10, %eax
    movl %eax, -8(%rbp)
    movl -8(%rbp), %edx
    movl %edx, %edi
    call putchar@PLT
    movl $0, %eax
    jmp main_exit
main_exit:
    movq %rbp, %rsp
    popq %rbp
    ret 
