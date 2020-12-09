# Lab 1

## E01

[Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html) 一定要读一下.



## Bootstrap

### 物理地址空间

```
+------------------+  <- 0xFFFFFFFF (4GB)
|      32-bit      |
|  memory mapped   |
|     devices      |
|                  |
/\/\/\/\/\/\/\/\/\/\

/\/\/\/\/\/\/\/\/\/\
|                  |
|      Unused      |
|                  |
+------------------+  <- depends on amount of RAM
|                  |
|                  |
| Extended Memory  |
|                  |
|                  |
+------------------+  <- 0x00100000 (1MB)
|     BIOS ROM     |
+------------------+  <- 0x000F0000 (960KB)
|  16-bit devices, |
|  expansion ROMs  |
+------------------+  <- 0x000C0000 (768KB)
|   VGA Display    |
+------------------+  <- 0x000A0000 (640KB)
|                  |
|    Low Memory    |
|                  |
+------------------+  <- 0x00000000

```

The first PCs, which were based on the 16-bit Intel 8088 processor, were only capable of addressing 1MB of physical memory. The physical address space of an early PC would therefore start at 0x00000000 but end at 0x000FFFFF instead of 0xFFFFFFFF. The 640KB area marked "Low Memory" was the only random-access memory (RAM) that an early PC could use; in fact the very earliest PCs only could be configured with 16KB, 32KB, or 64KB of RAM! 

The 384KB area from 0x000A0000 through 0x000FFFFF was reserved by the hardware for special uses such as video display buffers and firmware held in non-volatile memory. The most important part of this reserved area is the Basic Input/Output System (BIOS), which occupies the 64KB region from 0x000F0000 through 0x000FFFFF. In early PCs the BIOS was held in true read-only memory (ROM), but current PCs store the BIOS in updateable flash memory. The BIOS is responsible for performing basic system initialization such as activating the video card and checking the amount of memory installed. After performing this initialization, the BIOS loads the operating system from some appropriate location such as floppy disk, hard disk, CD-ROM, or the network, and passes control of the machine to the operating system.

When Intel finally "broke the one megabyte barrier" with the 80286 and 80386 processors, which supported 16MB and 4GB physical address spaces respectively, the PC architects nevertheless preserved the original layout for the low 1MB of physical address space in order to ensure backward compatibility with existing software. Modern PCs therefore have a "hole" in physical memory from 0x000A0000 to 0x00100000, dividing RAM into "low" or "conventional memory" (the first 640KB) and "extended memory" (everything else). In addition, some space at the very top of the PC's 32-bit physical address space, above all physical RAM, is now commonly reserved by the BIOS for use by 32-bit PCI devices.

Recent x86 processors can support more than 4GB of physical RAM, so RAM can extend further above 0xFFFFFFFF. In this case the BIOS must arrange to leave a second hole in the system's RAM at the top of the 32-bit addressable region, to leave room for these 32-bit devices to be mapped. Because of design limitations JOS will use only the first 256MB of a PC's physical memory anyway, so for now we will pretend that all PCs have "only" a 32-bit physical address space. But dealing with complicated physical address spaces and other aspects of hardware organization that evolved over many years is one of the important practical challenges of OS development. 

### BIOS

Why does QEMU start like this? This is how Intel designed the 8088 processor, which IBM used in their original PC. Because the BIOS in a PC is "hard-wired" to the physical address range 0x000f0000-0x000fffff, this design ensures that the BIOS always gets control of the machine first after power-up or any system restart - which is crucial because on power-up there is no other software anywhere in the machine's RAM that the processor could execute. The QEMU emulator comes with its own BIOS, which it places at this location in the processor's simulated physical address space. On processor reset, the (simulated) processor enters real mode and sets CS to 0xf000 and the IP to 0xfff0, so that execution begins at that (CS:IP) segment address. How does the segmented address 0xf000:fff0 turn into a physical address?

To answer that we need to know a bit about real mode addressing. In real mode (the mode that PC starts off in), address translation works according to the formula: physical address = 16 * segment + offset. So, when the PC sets CS to 0xf000 and IP to 0xfff0, the physical address referenced is:

	16 * 0xf000 + 0xfff0   # in hex multiplication by 16 is
	= 0xf0000 + 0xfff0     # easy--just append a 0.
	= 0xffff0 

0xffff0 is 16 bytes before the end of the BIOS (0x100000). Therefore we shouldn't be surprised that the first thing that the BIOS does is jmp backwards to an earlier location in the BIOS; after all how much could it accomplish in just 16 bytes? 


## E02

...

## E03

- Q1: At what point does the processor start executing 32-bit code？
	
	Q2: What exactly causes the switch from 16- to 32-bit mode?
  
	```
	# Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses 
  # identical to their physical addresses, so that the 
  # effective memory map does not change during the switch.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0
  
  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $PROT_MODE_CSEG, $protcseg
	```


- What is the last instruction of the boot loader executed?

	boot/main.c 的 bootmain 函数中的 `((void (*)(void)) (ELFHDR->e_entry))()` 跳转到 kernel.

- What is the first instruction of the kernel it just loaded?
 
	```
	.globl entry
	entry:
	movw	$0x1234,0x472			# warm boot
 	```

- Where is the first instruction of the kernel?
	
	第一条指令的地址在 `0x0010000c`. 可以通过 `readelf -h obj/kern/kernel` 查看.

	```c
	$ readelf -h obj/kern/kernel
	ELF Header:
		Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00 
		Class:                             ELF32
		Data:                              2's complement, little endian
		Version:                           1 (current)
		OS/ABI:                            UNIX - System V
		ABI Version:                       0
		Type:                              EXEC (Executable file)
		Machine:                           Intel 80386
		Version:                           0x1
		Entry point address:               0x10000c
		Start of program headers:          52 (bytes into file)
		Start of section headers:          1563544 (bytes into file)
		Flags:                             0x0
		Size of this header:               52 (bytes)
		Size of program headers:           32 (bytes)
		Number of program headers:         3
		Size of section headers:           40 (bytes)
		Number of section headers:         10
		Section header string table index: 9
	```

- How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
  
	ELF header 包含必要信息.

	- Start of program headers
	- Number of program headers
	- Size of program headers

## E04

...

## ELF

An ELF binary starts with a fixed-length ELF header, followed by a variable-length program header listing each of the program sections to be loaded. The C definitions for these ELF headers are in inc/elf.h. The program sections we're interested in are:

- .text: The program's executable instructions.

- .rodata: Read-only data, such as ASCII string constants produced by the C compiler. (We will not bother setting up the hardware to prohibit writing, however.)

- .data: The data section holds the program's initialized data, such as global variables declared with initializers like int x = 5;.

When the linker computes the memory layout of a program, it reserves space for uninitialized global variables, such as int x;, in a section called .bss that immediately follows .data in memory. C requires that "uninitialized" global variables start with a value of zero. Thus there is no need to store contents for .bss in the ELF binary; instead, the linker records just the address and size of the .bss section. The loader or the program itself must arrange to zero the .bss section. 

值得注意的是, .data 在 ELF(二进制可执行文件) 中是占据实际空间的, 毕竟这部分空间会实际加载到内存中. 而 .bss 保存的是未初始化的全局变量, 默认等于 0, 因此为了节省文件空间, .bss 在 ELF 中不占据实际空间, 而是记录起始地址和总大小, 当 loader(加载器) 将 ELF 加载进内存时, 会为 .bss 分配空间并将其空间初始化为 0.

The link address of a section is the memory address from which the section expects to execute. The linker encodes the link address in the binary in various ways, such as when the code needs the address of a global variable, with the result that a binary usually won't work if it is executing from an address that it is not linked for. (It is possible to generate **position-independent code** that does not contain any such absolute addresses. This is used extensively by modern shared libraries, but it has performance and complexity costs, so we won't be using it in 6.828.) 


## E05

如果 bootloader 的 linker addr 和 load addr 不一致, 那么肯定会导致寻址错误.

将 boot/Makefrag 中的 `-Ttext 0x7C00` 改为 `-Ttext 0x6C00`, 然后 使用 gdb 调试. 得到如下结果:

```c
The target architecture is assumed to be i8086
[f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
0x0000fff0 in ?? ()
+ symbol-file obj/kern/kernel
// 设置断点
(gdb) b *0x7c00		
Breakpoint 1 at 0x7c00
(gdb) c
Continuing.
[   0:7c00] => 0x7c00:	cli

Breakpoint 1, 0x00007c00 in ?? ()
// 省略中间单步
(gdb) si
[   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x6c32
0x00007c2d in ?? ()
(gdb) si
[   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x6c32
0x00007c2d in ?? ()
```

可以看出 ljmp 命令执行失败, 下面是  qemu 的报错:

```c
EAX=00000011 EBX=00000000 ECX=00000000 EDX=00000080
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00006f20
EIP=00007c2d EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
CS =0000 00000000 0000ffff 00009b00 DPL=0 CS16 [-RA]
SS =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
DS =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
FS =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
GS =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 
DR6=ffff0ff0 DR7=00000400
EFER=0000000000000000
Triple fault.  Halting for inspection via QEMU monitor.
```

## E06

kernel 的 program headers 如下:

```c
$ readelf -l obj/kern/kernel  

Elf file type is EXEC (Executable file)
Entry point 0x10000c
There are 3 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0xf0100000 0x00100000 0x1dfb0 0x1dfb0 R E 0x1000
  LOAD           0x01f000 0xf011e000 0x0011e000 0x15b3dc 0x19ddb0 RW  0x1000
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RWE 0x10

 Section to Segment mapping:
  Segment Sections...
   00     .text .rodata .stab .stabstr 
   01     .data .bss 
   02     
```

可以看出 kernel 的第一个 Segment 的 PhysAddr 是 0x00100000, VirtAddr 是 0xf0100000. bootloader 根据 PhysAddr 将 kernel 加载到内存中.

> 用户程序的 segment 的 PhysAddr 和 VirtAddr 一般是一样的, 因为用户程序已经由内核启用了页表, 加载器将用户程序加载到 VirtAddr.

前面已经知道了 kernel 的 entry point 是 0x10000c, 可见刚开始从 bootloader 进入 kernel 时, 使用的还是物理地址. 接着 kernel 会启用临时页表, 便开始使用虚拟地址了. 

因为 kernel 会启用页表, 所以它的 linkder addr 和 loader addr 是不一样的. kernel 会保证在启用页表之前的指令不会使用到虚拟地址, 因此即使两个地址不一样也不会导致问题.


实验中的问题, 0x00100000 开始的 8 个字的内容是:

- 刚进入 bootloader 时:
  
	```c
	(gdb) x/8x 0x00100000
	0x100000:	0x00000000	0x00000000	0x00000000	0x00000000
	0x100010:	0x00000000	0x00000000	0x00000000	0x00000000
	```

- 进入 kernel 时:
  
	gdb 的结果:

	```c
	(gdb) x/16xb 0x00100000
	0x100000:	0x02	0xb0	0xad	0x1b	0x00	0x00	0x00	0x00
	0x100008:	0xfe	0x4f	0x52	0xe4	0x66	0xc7	0x05	0x72
	```

	kernel.asm 的结果:

	```asm
	.globl entry
	entry:
		movw	$0x1234,0x472			# warm boot
	f0100000:	02 b0 ad 1b 00 00    	add    0x1bad(%eax),%dh
	f0100006:	00 00                	add    %al,(%eax)
	f0100008:	fe 4f 52             	decb   0x52(%edi)
	f010000b:	e4                   	.byte 0xe4

	f010000c <entry>:
	f010000c:	66 c7 05 72 04 00 00 	movw   $0x1234,0x472
	```

	kernel 文件的结果:
	
	```c
	$ hexdump -s 0x001000 -n 16 -C obj/kern/kernel
	00001000  02 b0 ad 1b 00 00 00 00  fe 4f 52 e4 66 c7 05 72  |.........OR.f..r|
	00001010
	```

## kernel 的初始虚拟内存

When you inspected the boot loader's link and load addresses above, they matched perfectly, but there was a (rather large) disparity between the kernel's link address (as printed by objdump) and its load address. Go back and check both and make sure you can see what we're talking about. (Linking the kernel is more complicated than the boot loader, so the link and load addresses are at the top of kern/kernel.ld.)

Operating system kernels often like to be linked and run at very high virtual address, such as 0xf0100000, in order to leave the lower part of the processor's virtual address space for user programs to use. The reason for this arrangement will become clearer in the next lab.

Many machines don't have any physical memory at address 0xf0100000, so we can't count on being able to store the kernel there. Instead, we will use the processor's memory management hardware to map virtual address 0xf0100000 (the link address at which the kernel code expects to run) to physical address 0x00100000 (where the boot loader loaded the kernel into physical memory). This way, although the kernel's virtual address is high enough to leave plenty of address space for user processes, it will be loaded in physical memory at the 1MB point in the PC's RAM, just above the BIOS ROM. This approach requires that the PC have at least a few megabytes of physical memory (so that physical address 0x00100000 works), but this is likely to be true of any PC built after about 1990.

**In fact, in the next lab, we will map the entire bottom 256MB of the PC's physical address space, from physical addresses 0x00000000 through 0x0fffffff, to virtual addresses 0xf0000000 through 0xffffffff respectively. You should now see why JOS can only use the first 256MB of physical memory.**

**For now, we'll just map the first 4MB of physical memory, which will be enough to get us up and running. We do this using the hand-written, statically-initialized page directory and page table in kern/entrypgdir.c.** For now, you don't have to understand the details of how this works, just the effect that it accomplishes. Up until kern/entry.S sets the **CR0_PG** flag, memory references are treated as physical addresses (strictly speaking, they're linear addresses, but boot/boot.S set up an identity mapping from linear addresses to physical addresses and we're never going to change that). Once CR0_PG is set, memory references are virtual addresses that get translated by the virtual memory hardware to physical addresses. **entry_pgdir translates virtual addresses in the range 0xf0000000 through 0xf0400000 to physical addresses 0x00000000 through 0x00400000, as well as virtual addresses 0x00000000 through 0x00400000 to physical addresses 0x00000000 through 0x00400000.** Any virtual address that is not in one of these two ranges will cause a hardware exception which, since we haven't set up interrupt handling yet, will cause QEMU to dump the machine state and exit (or endlessly reboot if you aren't using the 6.828-patched version of QEMU). 


## E07


```c
The target architecture is assumed to be i8086
[f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
0x0000fff0 in ?? ()
+ symbol-file obj/kern/kernel
(gdb) b *0x100025
Breakpoint 1 at 0x100025
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0x100025:	mov    %eax,%cr0

Breakpoint 1, 0x00100025 in ?? ()
// 页表生效前的结果, 0xf0100000 是无效的地址.
(gdb) x/x 0x100000
0x100000:	0x1badb002
(gdb) x/x 0xf0100000
0xf0100000 <_start-268435468>:	0x00000000
(gdb) si
=> 0x100028:	mov    $0xf010002f,%eax
0x00100028 in ?? ()
// 页表生效后的结果, 0xf0100000 和 0x00100000 指向相同的物理地址.
(gdb) x/x 0x100000
0x100000:	0x1badb002
(gdb) x/xw 0xf0100000
0xf0100000 <_start-268435468>:	0x1badb002
```

注释掉 `movl %eax, %cr0` 后, 发生错误:

```c
The target architecture is assumed to be i8086
[f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
0x0000fff0 in ?? ()
+ symbol-file obj/kern/kernel
(gdb) b *0x00100020
Breakpoint 1 at 0x100020
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0x100020:	or     $0x80010001,%eax

Breakpoint 1, 0x00100020 in ?? ()
(gdb) si
=> 0x100025:	mov    $0xf010002c,%eax
0x00100025 in ?? ()
(gdb) si
=> 0x10002a:	jmp    *%eax
0x0010002a in ?? ()
(gdb) si
=> 0xf010002c <relocated>:	add    %al,(%eax)
relocated () at kern/entry.S:75
75		movl	$0x0,%ebp			# nuke frame pointer
(gdb) si
Remote connection closed
```

```
qemu: fatal: Trying to execute code outside RAM or ROM at 0xf010002c

EAX=f010002c EBX=00010094 ECX=00000000 EDX=00000000
ESI=00010094 EDI=00000000 EBP=00007bf8 ESP=00007bec
EIP=f010002c EFL=00000086 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     00007c4c 00000017
IDT=     00000000 000003ff
CR0=00000011 CR2=00000000 CR3=00126000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=80010011 CCO=EFLAGS
EFER=0000000000000000
FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80
FPR0=0000000000000000 0000 FPR1=0000000000000000 0000
FPR2=0000000000000000 0000 FPR3=0000000000000000 0000
FPR4=0000000000000000 0000 FPR5=0000000000000000 0000
FPR6=0000000000000000 0000 FPR7=0000000000000000 0000
XMM00=00000000000000000000000000000000 XMM01=00000000000000000000000000000000
XMM02=00000000000000000000000000000000 XMM03=00000000000000000000000000000000
XMM04=00000000000000000000000000000000 XMM05=00000000000000000000000000000000
XMM06=00000000000000000000000000000000 XMM07=00000000000000000000000000000000
make: *** [GNUmakefile:196: qemu-nox-gdb] Aborted (core dumped)
```

## E08



- Explain the following from console.c:
  
	```c
	if (crt_pos>= CRT_SIZE) {
					int i;
					memcpy(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
					for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
									crt_buf[i] = 0x0700 | ' ';
					crt_pos -= CRT_COLS;
	}
	```

	作用: 当 crt_buf 满了时, 将当前屏幕的内容往上移动一行, 相当于屏幕最下方空出一行, 然后将最下方那行的内容清空.


## E09

在 kern/entry.S 中:

```asm
relocated:

	# Clear the frame pointer register (EBP)
	# so that once we get into debugging C code,
	# stack backtraces will be terminated properly.
	movl	$0x0,%ebp			# nuke frame pointer

	# Set the stack pointer
	movl	$(bootstacktop),%esp
...

.data
###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:
```

在 .data 预留了 KSTKSIZE 的空间作为内核栈使用. 具体的 bootstacktop 等于 0xf0126000.




## E10

主要目的是为了熟悉汇编调用函数时栈的变化. 这个我已经熟悉了.

## E11

关键有两点:

- call 会将 PC 推入栈中.

- 进入新函数后, 首先将 %ebp 推入栈中, 然后将 %ebp 设置为新的 %esp.

根据这两点可以顺着 %ebp 不断回溯函数调用链.

## E12

按照文档实现.

## 总结

- 对 BIOS 的代码完全看不懂， 参考了这个 [repo](https://github.com/jtyuan/JOS2014/tree/lab1) 的笔记才稍微有点了解。

- 花了两三天时间才基本完成这个 lab，熟悉了从 BIOS，bootloader 到 kernel 的启动过程，知道了实模式和保护模式的概念，之前只知道类似的虚拟内存地址和页表的概念。

- 知道了 I/O 操作， in/out等命令。I/O 设备有自己的地址空间，CPU通过地址标识（port）和I/O设备进行通信。
  
- 知道了 C 语言中可变长度参数的实现方式，va_* 函数（宏）是编译器自带的。

- 知道了 printf 的底层原理，当然因为硬件兼容性等原因实际上会更加复杂。

	如果printf函数的fmt参数中的格式化字符的数量超过可变参数的数量，那么printf会输出栈上的“非法数据”。

	例如：`printf("%d %d %d", a)`，后面的两个`%d`会输出任意数据。