# Lab 1

The original lab material can be found [here](http://cs492virt.kaist.ac.kr/lab1.html)

### Exercise 3
> At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?

```asm
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0
 
  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $PROT_MODE_CSEG, $protcseg

  .code32                     # Assemble for 32-bit mode
protcseg:
  (the code below is omitted)
```
After the boot loader enables protection mode and executes the long-jump operation, the processor starts executing 32-bit code. The above code is from `boot.S`, line 93-102. 

From line 93, the boot loader enables protection mode by setting flag `CR0_PE_ON` to CR0 register. 
The reason we need long-jump operation in line 99 is because after the label `protcseg`, the address of codes are expressed in 32-bit mode. 

Thus, the boot loader needs to use long-jump operation, rather than the ordinary jump operation. Below log shows that gdb notifies the execution mode has changed into 32-bit mode.
```shell
(gdb) si
[f000:d190]    0xfd190:	ljmpl  $0x8,$0xfd198
0x0000d190 in ?? ()
3: $cs = 61440
2: $ip = void
1: $pc = (void (*)()) 0xd190
(gdb) si
The target architecture is assumed to be i386
=> 0xfd198:	mov    $0x10,%eax
0x000fd198 in ?? ()
3: $cs = 8
2: $ip = void
1: $pc = (void (*)()) 0xfd198
```

> What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?

The following text is the tailed disassembly of bootmain, which was from `objdump -d obj/boot/boot.out`.
```
    7ddb:	b8 00 70 00 00       	mov    $0x7000,%eax
    7de0:	89 c3                	mov    %eax,%ebx
    7de2:	ff 15 18 00 01 00    	call   *0x10018
    7de8:	ba 00 8a 00 00       	mov    $0x8a00,%edx
    7ded:	b8 00 8a ff ff       	mov    $0xffff8a00,%eax
    7df2:	66 ef                	out    %ax,(%dx)
    7df4:	b8 00 8e ff ff       	mov    $0xffff8e00,%eax
    7df9:	66 ef                	out    %ax,(%dx)
    7dfb:	eb fe                	jmp    7dfb <bootmain+0x76>
```
The binaries below `7de8` are executed when the bootloader fails to load the OS.
We can conclude that the last instruction executed by the bootloader is `7de8`, which is `call *0x10018`.
The instruction calls the main function of OS.

From `readelf -e obj/kern/kernel`, we can see that the entry point of the kernel ELF is `0x100000`. If we `objdump` the kernel and see what's there in `0x100000`, we can see that the following instruction lies there.
```
0000000000100000 <_head64>:
  100000:	b8 00 70 10 00       	mov    $0x107000,%eax
  100005:	89 18                	mov    %ebx,(%rax)
  100007:	66 c7 05 72 04 00 00 	movw   $0x1234,0x472(%rip)        # 100482 <verify_cpu_no_longmode+0x36f>
```
Thus, we can conclude that the first instruction kernel executes is `mov $0x107000,%eax`.

> How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?

The size of disk sector is 512 byte, as defined on line 32 of main.c. 
When the boot loader loads kernel, it first reads ELF header of kernel. The ELF header takes 8 sectors. 
After read ELF header of kernel, the boot loader loads each segments of kernel.

I performed `elfread -a` to `obj/kern/kernel`, which is the actual kernel and I could get the following result.
There were 18 sections on kernel. I noticed this by inspecting `e_phnum` value of ELF header.
Each section is stored on disk, aligned by 512 byte which is the actual sector size.

Thus, we can conclude that the boot loader reads sum(ceil(sector_size)/SECTSIZE) sectors to fetch entire kernel. 
As I perform the calculation above, I could conclude that 423 sectors are read to fetch the entire kernel.

### Exercise 6
> Reset the machine (exit QEMU/GDB and start them again). Examine the 8 words of memory at 0x00100000 at the point the BIOS enters the boot loader, and then again at the point the boot loader enters the kernel. Why are they different? What is there at the second breakpoint? (You do not really need to use QEMU to answer this question. Just think.)

Right after the boot loader is started to executed, the kernel is not yet loaded.
Thus, there are only zero values in 0x100000 and so forth.
On the other hand, when the boot loader enters the kernel, the boot loader has fully read the ELF of kernel.
Thus, there are hex values in 0x100000 and its nearby area, which are actually instuctions from `kern/bootstrap.s`.

### Exercise 7
> Use QEMU and GDB to trace into the early JOS kernel boot code (in the kern/boostrap.S directory) and find where the new virtual-to-physical mapping takes effect. 

The new virtual-to-physical mapping takes effect when the below instructions are executed.
```asm
    # enable paging 
    movl %cr0,%eax
    orl $CR0_PE,%eax
    orl $CR0_PG,%eax
    orl $CR0_AM,%eax
    orl $CR0_WP,%eax
    orl $CR0_MP,%eax
    movl %eax,%cr0
```
By setting CR0 registers with some flags (e.g. PE for enabling protected mode), the new mapping takes effect.
From this, the linear address is not directly mapped into the physical address.

> Then examine the Global Descriptor Table (GDT) that the code uses to achieve this effect, and make sure you understand what's going on.

```asm
   0x1000d5 <_head64+213>:	mov    $gdtdesc_64,%eax
   0x1000da <_head64+218>:	lgdtl  (%eax)
```
The GDT description is defined under the address `$gdtdesc_64`. By referencing the address, we can see GDT entries in `gdt_64`.
```asm
gdt_64:
    SEG_NULL
    .quad  0x00af9a000000ffff            #64 bit CS
    .quad  0x00cf92000000ffff            #64 bit DS
```
Each entry in the table defines the behavior of segments. (e.g. base address, limit address ...)

> What is the first instruction after the new mapping is established that would fail to work properly if the old mapping were still in place? Comment out or otherwise intentionally break the segmentation setup code in kern/entry.S, trace into it, and see if you were right.

```asm
    movabs   $gdtnull_64,%rax
    lgdt     (%rax)
    (omitted)
    movabs  $relocated,%rax
    pushq   %rax
    lretq
relocated:
=>	movq	$0x0,%rbp			# nuke frame pointer
```

I disabled the segmentation setup by defining new GDT `gdtnull_64`, which only contains segment descript `SEG_NULL`.
By pushing the address of `relocated` label and returning, the kernel tries to execute instructions after `relocated` label. 
However, the kernel cannot execute the instruction at the label because it is not executable, just as defined in the GDT. 

### Exercise 9
> Determine where the kernel initializes its stack, and exactly where in memory its stack is located. How does the kernel reserve space for its stack? And at which "end" of this reserved area is the stack pointer initialized to point to?
In `kern/entry.S`, we can see the empty stack initialization as described below.
```asm
	movq	$0x0,%rbp			# nuke frame pointer

	# Set the stack pointer
	movabs	$(bootstacktop),%rax
	movq  %rax,%rsp
    (omitted assemblies)
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:
```
Thus, we can see that the address of bootstacktop is the exact location where our stack is.
By disassembling the kernel binary, I could know that the location is `$0x800421b000`.
The kernel reserves stack area(for KSTKSIZE = 5 PGs), under the .data section(with some alignments).
Initially, the top of the stack will point border of the data section.

### Exercise 10
> How many 64-bit words does each recursive nesting level of test_backtrace push on the stack, and what are those words?
```shell
Breakpoint 5, test_backtrace (x=3) at kern/init.c:21
21	{
(gdb) i r
(omitted registers)
rdi            0x4	4 # => 0x3  3
rbp            0x800421afe0	0x800421afe0 # => 0x800421afc0 0x800421afc0
rsp            0x800421afc8	0x800421afc8 # => 0x800421afa8 0x800421afa8
```
The difference of `rsp` is 0x20. Thus, we can conclude that one recursive step pushed 4 qwords. I inspected what has been pushed when taking a recursive step.
```shell
(gdb) x/8x $rsp
0x800421afa8:	0x0420009d	0x00000080	0x0421ce00	0x00000080
0x800421afb8:	0x0421ce00	0x00000004	0x0421afe0	0x00000080
(gdb) disas test_backtrace
   0x000000800420009b <+67>:	callq  *%rax
=> 0x000000800420009d <+69>:	jmp    0x80042000ba <test_backtrace+98>
```
They are: return address(0x800420009d), function argument(4), last rbp(0x800421afe0), and some abandoned value (0x0421ce00)

Note that exercise 12 does not work :(


