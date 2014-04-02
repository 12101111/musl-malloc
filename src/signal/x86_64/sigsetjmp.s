/* Copyright 2011-2012 Nicholas J. Kain, licensed under standard MIT license */
.global sigsetjmp
.global __sigsetjmp
.type sigsetjmp,@function
.type __sigsetjmp,@function
sigsetjmp:
__sigsetjmp:
	andl %esi,%esi
	movq %rsi,64(%rdi)
	jz 1f
	pushq %rdi
	leaq 72(%rdi),%rdx
	xorl %esi,%esi
	movl $2,%edi
	call sigprocmask
	popq %rdi
1:	jmp setjmp
