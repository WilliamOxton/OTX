	.text
	.globl	springboard
	.align	0x1000
	.type	springboard,@function
springboard:                            # @springboard
	.cfi_startproc
	retq
							### n->s
							# atsgx.n2s.src S会用到rax
	.globl	atsgx.n2s.src    
atsgx.n2s.src:
	mov %rax, %r14
atsgx.n2s.src.body:
	xbegin  atsgx.n2s.src.body
	mov %r14, %rax
	jmp *%r15
							#atsgx.n2s.none S不会用到rax
	.globl atsgx.n2s.none 	 
atsgx.n2s.none:
	mov %rax, %r14
atsgx.n2s.none.body:
	xbegin  atsgx.n2s.none.body
	jmp *%r15
							### s->n
							#atsgx.s2n.dest S输出值到rax
	.globl atsgx.s2n.dest    
atsgx.s2n.dest:
	xend
	jmp *%r15
							#atsgx.s2n.none S不输出值到rax
	.globl atsgx.s2n.none    
atsgx.s2n.none:
	xend
	mov %rax, %r14
	jmp *%r15
							### s->s
							#atsgx.s2s.dest.src S1输出值到rax，S2使用rax的值
	.globl atsgx.s2s.dest.src
atsgx.s2s.dest.src:
	xend
	mov %rax, %r14
atsgx.s2s.dest.src.body:
	xbegin atsgx.s2s.dest.src.body
	mov %r14, %rax
	jmp *%r15
							#atsgx.s2s.dest.none S1输出到rax，S2不使用rax的值
	.globl atsgx.s2s.dest.none
atsgx.s2s.dest.none:
	xend
	mov %rax, %r14
atsgx.s2s.dest.none.body:
	xbegin atsgx.s2s.dest.none.body
	jmp *%r15
							#atsgx.s2s.none.src S1不输出值到rax，S2使用rax的值
	.globl atsgx.s2s.none.src
atsgx.s2s.none.src:
	xend
atsgx.s2s.none.src.body:
	xbegin  atsgx.s2s.none.src.body
	mov %r14, %rax
	jmp *%r15
							#atsgx.s2s.none.none S1不输出值到rax，S2不使用rax的值
	.globl atsgx.s2s.none.none
atsgx.s2s.none.none:
	xend
atsgx.s2s.none.none.body:
	xbegin atsgx.s2s.none.none.body
	jmp *%r15
							### call 直接调用外部函数｜间接调用
							# atsgx.ret 函数返回
	.globl atsgx.ret
atsgx.ret:
	xend
	ret
							# atsgx.call.s2e.none 被调用的函数不会输出值到rax
	.globl atsgx.call.s2e.none
atsgx.call.s2e.none:
	xend
	call *%r15
atsgx.call.s2e.none.ret:
	xbegin atsgx.call.s2e.none.ret
	ret
							# atsgx.call.s2e.rax 被调用的函数会输出值到rax
	.globl atsgx.call.s2e.rax
atsgx.call.s2e.rax:
	xend
	call *%r15
	mov %rax, %r14
atsgx.call.s2e.rax.ret:
	xbegin atsgx.call.s2e.rax.ret
	mov %r14, %rax
	ret
							# atsgx.call.s2s.rax 直接调用，并且函数会输出值到rax
	.globl atsgx.call.s2s.rax
atsgx.call.s2s.rax:
	xend
atsgx.call.s2s.rax.body:
	xbegin atsgx.call.s2s.rax.body
	call *%r15
	mov %rax, %r14
atsgx.call.s2s.rax.ret:			
	xbegin atsgx.call.s2s.rax.ret
	mov %r14, %rax
	ret
							# atsgx.call.s2s.none 直接调用，并且函数不会输出值到rax
	.globl atsgx.call.s2s.none
atsgx.call.s2s.none:
	xend
atsgx.call.s2s.none.body:
	xbegin atsgx.call.s2s.none.body
	call *%r15
atsgx.call.s2s.none.ret:			
	xbegin atsgx.call.s2s.none.ret
	ret
							# atsgx.tailcall.s2e 
	.globl atsgx.tailcall.s2e
atsgx.tailcall.s2e:
	xend
	jmp *%r15
								# atsgx.tailcall.s2s 
	.globl atsgx.tailcall.s2s
atsgx.tailcall.s2s:
	xend
atsgx.tailcall.s2s.body:
	xbegin atsgx.tailcall.s2s.body
	jmp *%r15
	
							# springboard end
.Lfunc_end0:
	.align	0x1000
	.size	springboard, .Lfunc_end0-springboard
	.cfi_endproc
	.section	".note.GNU-stack","",@progbits
