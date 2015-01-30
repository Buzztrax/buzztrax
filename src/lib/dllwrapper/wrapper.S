.section .data
caller_return:
	.long 0
.global wrapper_target
wrapper_target:	
	.long null_call

#undef __i686 /* gcc define gets in our way */
	.section .gnu.linkonce.t.__i686.get_pc_thunk.bx,"ax",@progbits
.globl __i686.get_pc_thunk.bx
	.hidden	__i686.get_pc_thunk.bx
	.type	__i686.get_pc_thunk.bx,@function
__i686.get_pc_thunk.bx:
	movl (%esp), %ebx
	ret

.section .text
.globl null_call
	.type null_call, @function
	.balign 16,0x90
null_call:
	ret
.globl wrapper
	.type wrapper, @function
	.balign 16,0x90
wrapper:
	pushl $0
	pusha			# store registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI) 
	pushf			# store flags
	
	push %ebp		# set up a stack frame 
	movl %esp, %ebp

	call __i686.get_pc_thunk.bx
	addl $_GLOBAL_OFFSET_TABLE_, %ebx

	leal 4(%ebp), %eax	# push flags addr
	push %eax
	leal 8(%ebp), %eax	# push registers addr
	push %eax
	
	leal 44(%ebp), %edx	
	movl (%ebp), %eax
	subl %edx, %eax
	push %eax
	push %edx
	
	call report_func@PLT	# report entry
	
	test %eax, %eax
	jnz .Ldone

	movl 44(%ebp), %eax	# switch return addresses
	movl %eax, caller_return@GOTOFF(%ebx)
	leal .Lwrapper_return@GOTOFF(%ebx), %eax
	movl %eax, 40(%ebp)

	movl wrapper_target@GOT(%ebx), %eax
	movl (%eax), %eax
	mov %eax, 40(%ebp)	# wrapper_target should return at .Lwrapper_return

	leave			# restore %esp, %ebp
	popf			# restore flags
	popa			# restore registers

	ret			# fake 'return' to wrapper_target actually

	.balign 16, 0x90
.Lwrapper_return:	
	pushl $0
	pusha			# more for reference sake here
	pushf
	
	push %ebp		# set up a stack frame
	movl %esp, %ebp

	call __i686.get_pc_thunk.bx
	addl $_GLOBAL_OFFSET_TABLE_, %ebx
	movl caller_return@GOTOFF(%ebx), %eax
	movl %eax, 40(%ebp)	# restore the original return address

	leal 4(%ebp), %eax	# push flags addr
	push %eax
	leal 8(%ebp), %eax	# push registers addr
	push %eax

	leal 40(%ebp), %edx	# push stack top address (relative to our entry)
	movl (%ebp), %eax
	subl %edx, %eax		# calculate difference between entry and previous frame
	push %eax
	push %edx

	call report_func_ret@PLT# report the return information (same args)
.Ldone:	

	leave
	popf
	popa
	ret

.section .note.GNU-stack,"",@progbits

