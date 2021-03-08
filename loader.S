#include <boot.h>
#include <msr.h>

	.code16
	.text
start16:
	ljmp	$0, $1f
1:	
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	mov	$BOOT_STACK16_BASE, %sp

	// Enable A20.
	mov	$0x2401, %ax
	int	$0x15
	
	// Detect Memory Layout.
	mov	$BOOT_E820_BASE, %edi
	xor	%ebx, %ebx
1:
	mov	$100, %ecx
	mov	$0x534d4150, %edx
	mov	$0xe820, %ax
	add	$4, %edi
	int	$0x15
	jc	1f
	mov	%ecx, -4(%edi)
	add	%ecx, %edi
	test	%ebx, %ebx
	jnz	1b
1:
	sub	$BOOT_E820_BASE, %edi
	mov	%edi, BOOT_E820_SIZE
	
	// Clear 4 pages
	mov	$BOOT_PAGE_TBL_BASE, %si
	mov	%si, %di
	mov	$4096, %ecx
	xor	%eax, %eax
	cld
	rep	stosl
	// PML4E (1 entry)
	mov	%si, %di
	lea	0x1000(%di), %eax
	or	$0x7, %eax
	mov	%eax, 0x0(%di)
	lea	0x1000(%di), %eax
	or	$0x1f, %eax
	mov	%eax, 0x8(%di)
	// PDP (1GiB * 512 entries)
	lea	0x1000(%di), %di
	mov	$0x87, %eax
	xor	%edx, %edx
	mov	$512, %ecx
1:
	mov	%eax, (%di)
	mov	%edx, 4(%di)
	add	$0x40000000, %eax
	adc	$0, %edx
	add	$8, %di
	dec	%ecx
	jnz	1b
	
	// No interrupt
	mov	$0xff, %al
	outb	%al, $0xa1
	outb	%al, $0x21

	// Switch to long mode.
	cli
	mov	$BOOT_CR4, %eax
	mov	%eax, %cr4
	xor	%eax, %eax
	mov     $BOOT_PAGE_TBL_BASE, %ax
	mov	%eax, %cr3
	mov	$0xc0000080, %ecx
	mov	$BOOT_EFER, %eax
	xor	%edx, %edx
	wrmsr
	mov	$BOOT_CR0, %eax
	mov	%eax, %cr0
	lidt	idt
	lgdt	gdt
	ljmpl	$8, $1f
	
	.code64
1:
	mov	$0x10, %eax
	mov	%eax, %ds
	mov	%eax, %es
	mov	%eax, %fs
	mov	%eax, %gs
	mov	%eax, %ss
	mov	$BOOT_BSP_STACK_BASE, %esp
	
	// Clear BSS.
	mov	$__bss_begin, %edi
	mov	$__bss_end, %ecx
	sub	$__bss_begin, %ecx
	xor	%al, %al
	cld
	rep	stosb
	
	call	init
	jmp	stop
	
	.org	0x1fe
	.byte	0x55, 0xaa

	// AP starts at 0x8000.
	.org	0x400
	.code16
start_ap:	
	jmp	$0, $1f
1:	
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	xor	%sp, %sp

	// Switch to long mode.
	cli
	mov	$BOOT_CR4, %eax
	mov	%eax, %cr4
	xor	%eax, %eax
	mov     $BOOT_PAGE_TBL_BASE, %ax
	mov	%eax, %cr3
	mov	$0xc0000080, %ecx
	mov	$BOOT_EFER, %eax
	xor	%edx, %edx
	wrmsr
	mov	$BOOT_CR0, %eax
	mov	%eax, %cr0
	lidt	idt
	lgdt	gdt
	ljmpl	$8, $1f
	
	.code64
1:
	mov	$0x10, %eax
	mov	%eax, %ds
	mov	%eax, %es
	mov	%eax, %fs
	mov	%eax, %gs
	mov	%eax, %ss
	
	// x2APIC (Enable & Get ID)
	mov	$MSR_IA32_APIC_BASE, %rcx
	rdmsr
	or	$0xc00, %rax
	wrmsr
	mov	$MSR_X2APIC_ID, %rcx
	rdmsr
	mov	%rax, %rdi
	
	// Get stack.
	xor	%rsi, %rsi
	mov	stacklist, %rbx
1:	
	cmp	%rdi, (%rbx,%rsi,1)
	je	1f
	add	$16, %rsi
	jmp	1b
1:	
	add	$8, %rsi
	mov	(%rbx,%rsi,1), %rax
	mov	%rax, %rsp
	call	init_ap
	jmp	stop

stop:	
	hlt
	jmp	stop
	
idt:
	.short	0
	.byte	0
gdt:
	.short	gdt_size - 1
	.short	gdt
	.long	0
	.quad	0x00af9b000000ffff # 64-bit code segment
	.quad	0x00cf93000000ffff # 64-bit data segment
gdt_size = . - gdt
