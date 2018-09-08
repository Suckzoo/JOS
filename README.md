# Lab 1

The original lab material can be found [here](http://cs492virt.kaist.ac.kr/lab1.html)

### Exercise 3
##### At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?
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
After the boot loader enables protection mode and executes the long-jump operation, the processor starts executing 32-bit code. The above code is from `boot.S`, line 93-102. From line 93, the boot loader enables protection mode by setting flag `CR0_PE_ON` to CR0 register. The reason we need long-jump operation in line 99 is because after the label `protcseg`, the address of codes are expressed in 32-bit mode. Thus, the boot loader needs to use long-jump operation, rather than the ordinary jump operation. Below log shows that gdb notifies the execution mode has changed into 32-bit mode.
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

##### What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
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


##### How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
The size of disk sector is 512 byte, as defined on line 32 of main.c. 
When the boot loader loads kernel, it first reads ELF header of kernel. The ELF header takes 8 sectors. 
After read ELF header of kernel, the boot loader loads each segments of kernel.
I performed `elfread -a` to `obj/kern/kernel`, which is the actual kernel and I could get the following result.
There were 18 sections on kernel. I noticed this by inspecting `e_phnum` value of ELF header.
Each section is stored on disk, aligned by 512 byte which is the actual sector size.
Thus, we can conclude that the boot loader reads sum(ceil(sector_size)/SECTSIZE) sectors to fetch entire kernel. 
//TODO: align 맞춰서 계산하기

### Exercise 6
##### Reset the machine (exit QEMU/GDB and start them again). Examine the 8 words of memory at 0x00100000 at the point the BIOS enters the boot loader, and then again at the point the boot loader enters the kernel. Why are they different? What is there at the second breakpoint? (You do not really need to use QEMU to answer this question. Just think.)

### Exercise 7
##### What is the first instruction after the new mapping is established that would fail to work properly if the old mapping were still in place? Comment out or otherwise intentionally break the segmentation setup code in kern/entry.S, trace into it, and see if you were right.

### Exercise 9
##### Determine where the kernel initializes its stack, and exactly where in memory its stack is located. How does the kernel reserve space for its stack? And at which "end" of this reserved area is the stack pointer initialized to point to?

### Exercise 10
##### How many 64-bit words does each recursive nesting level of test_backtrace push on the stack, and what are those words?