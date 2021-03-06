/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Olivier Gruber (olivier dot gruber at acm dot org)
*/

#define DIRECT_BOOT_EAX_MAGIC	0x6A6F7362
	
/* STA_ macros are for segment type values */
#define STA_A (1 << 0)	/* Accessed */
#define STA_W (1 << 1)	/* Writable (for data segments) */
#define STA_E (1 << 2)	/* Expand down (for data segments) */
#define STA_X (1 << 3)	/* 1 = Code segment (executable) */
#define STA_R (1 << 1)	/* Readable (for code segments) */
#define STA_C (1 << 2)	/* Conforming (for code segments) */
	
#define SEG32_ASM(type, base, lim)					\
    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);			\
    .byte (((base) >> 16) & 0xff), (0x90 | (type)),			\
	  (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)


.set PROT_MODE_CSEG,0x8		# code segment selector
.set PROT_MODE_DSEG,0x10        # data segment selector
.set CR0_PE_ON,0x1		# protected mode enable flag
	
###################################################################################
# ENTRY POINT	
#   This code should be stored in the first sector of the hard disk.
#   After the BIOS initializes the hardware on startup or system reset,
#   it loads this code at physical address 0x7c00 - 0x7d00 (512 bytes).
#   Then the BIOS jumps to the beginning of it, address 0x7c00,
#   while running in 16-bit real-mode (8086 compatibility mode).
#   The Code Segment register (CS) is initially zero on entry.
#	
# This code switches into 32-bit protected mode so that all of
# memory can be accessed, then calls into our loader, written in C.
###################################################################################
	
.global start					# Entry point
start:		.code16				# This runs in real mode
		cli				# Disable interrupts
		cld				# String operations increment

		# Set up the important data segment registers (DS, ES, SS).
		xorw	%ax,%ax			# Segment number zero
		movw	%ax,%ds			# -> Data Segment
		movw	%ax,%es			# -> Extra Segment
		movw	%ax,%ss			# -> Stack Segment

		# Set up the stack pointer, growing downward from 0x7c00.
		movw	$start,%sp         	# Stack Pointer
	
#### Enable A20:
####   For fascinating historical reasons (related to the fact that
####   the earliest 8086-based PCs could only address 1MB of physical memory
####   and subsequent 80286-based PCs wanted to retain maximum compatibility),
####   physical address line 20 is tied to low when the machine boots.
####   Obviously this a bit of a drag for us, especially when trying to
####   address memory above 1MB.  This code undoes this.
	
seta20.1:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.1		# Yes
		movb	$0xd1,%al		# Command: Write
		outb	%al,$0x64		#  output port
seta20.2:	inb	$0x64,%al		# Get status
		testb	$0x2,%al		# Busy?
		jnz	seta20.2		# Yes
		movb	$0xdf,%al		# Enable
		outb	%al,$0x60		#  A20

#### Before going to protected mode, query memory size
#### Taken from Linux 2.6.6's arch/i386/boot/setup.S
		stc			# Fix to work around buggy
		xorw	%cx,%cx		# BIOSes which don't clear/set
		xorw	%dx,%dx		# carry on pass/error of
					# e801h memory size call
					# or merely pass cx,dx though
					# without changing them.
		movw	$0xe801,%ax
		int	$0x15
		jc	nobiosmem

		cmpw	$0x0,%cx	# Kludge to handle BIOSes
		jne	memdetected	# which report their extended
		cmpw	$0x0,%dx	# memory in AX/BX rather than
		jne	memdetected	# CX/DX. The spec I have read
		movw	%ax,%cx		# seems to indicate AX/BX
		movw	%bx,%dx		# are more reasonable anyway...
		jmp	memdetected

nobiosmem:	xorw	%cx,%cx
		xorw	%dx,%dx
memdetected:

#### Switch from real to protected mode	
####     The descriptors in our GDT allow all physical memory to be accessed.
####     Furthermore, the descriptors have base addresses of 0, so that the
####     segment translation is a NOP, ie. virtual addresses are identical to
####     their physical addresses.  With this setup, immediately after
####	 enabling protected mode it will still appear to this code
####	 that it is running directly on physical memory with no translation.
####	 This initial NOP-translation setup is required by the processor
####	 to ensure that the transition to protected mode occurs smoothly.
	
real_to_prot:	cli				# Mandatory since we dont set up an IDT
		lgdt	gdtdesc			# load GDT -- mandatory in protected mode
		movl	%cr0, %eax		# turn on protected mode
		orl	$CR0_PE_ON, %eax	# 
		movl	%eax, %cr0		# 
		### CPU magic: jump to relocation, flush prefetch queue, and reload %cs
		### Has the effect of just jmp to the next instruction, but simultaneous
		### loads CS with $PROT_MODE_CSEG.
		ljmp	$PROT_MODE_CSEG, $protcseg
	
#### we are in 32-bit protected mode (hence the .code32)
.code32
protcseg:
		# Set up the protected-mode data segment registers
		movw	$PROT_MODE_DSEG, %ax	# Our data segment selector
		movw	%ax, %ds		# -> DS: Data Segment
		movw	%ax, %es		# -> ES: Extra Segment
		movw	%ax, %fs		# -> FS
		movw	%ax, %gs		# -> GS
		movw	%ax, %ss		# -> SS: Stack Segment

		movl	$0xffff,%eax
		andl	%eax,%edx		# clip to 16 bits
		shll	$6,%edx			# go from 64K to 1K chunks
		andl	%eax,%ecx		# clip to 16 bits
		addl	%edx,%ecx		# add results

		pushl	%ecx			# Push the amount of extended memory
		pushl	$DIRECT_BOOT_EAX_MAGIC
		call	diskboot		# finish the boot load from C.
						# diskboot() should not return
spin:		jmp	spin			# ..but in case it does, spin
	
.p2align 2					# force 4 byte alignment
gdt:
	.quad 0					# null segment
	SEG32_ASM(STA_X|STA_R, 0, 0xffffffff)	# code seg
	SEG32_ASM(STA_W, 0, 0xffffffff)		# data seg
	
gdtdesc:
	.word	0x17			# sizeof(gdt) - 1
	.long	gdt			# address gdt
