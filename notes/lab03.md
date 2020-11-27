# Lab03

## E01

使用`boot_alloc`分配响应的空间.

## E02

重点是 `load_icode` 函数, 为了方便加载 program segment 到对应的虚拟内存地址, 可以将页表设置为当前 env 的页表.

按照这里的实现方式, 可以看出为什么 program segment 按照 PGSIZE 对齐. 各个 program segment 的区域不会出现在同一个页内, 换句话说, program segment 的内存是按页分配的, 有可能最后一页只使用了其中一部分, 但是另一个 program segment 不会重复使用该页的剩余部分.

下面是 `obj/user/hello` 的 program headers 内容:

```
Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0x00200000 0x00200000 0x03225 0x03225 RW  0x1000
  LOAD           0x005020 0x00800020 0x00800020 0x010f3 0x010f3 R E 0x1000
  LOAD           0x007000 0x00802000 0x00802000 0x0002c 0x00030 RW  0x1000
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RWE 0x10
```

注意第2个PH的va和memsz分别是`0x00800020` 和 `0x010f3`, 如果只分配所需内存的话, 下一个可以利用的地址应该是`0x00801113`, 然后第 3 个 PH 的 va 却是 `0x00802000`, 这个地址等于 `ROUNDUP(0x00801113, PGSIZE)`.

**triple fault**

>Once you are done you should compile your kernel and run it under QEMU. If all goes well, your system should enter user space and execute the hello binary until it makes a system call with the int instruction. At that point there will be trouble, since JOS has not set up the hardware to allow any kind of transition from user space into the kernel. When the CPU discovers that it is not set up to handle this system call interrupt, it will generate a general protection exception, find that it can't handle that, generate a double fault exception, find that it can't handle that either, and finally give up with what's known as a "triple fault". Usually, you would then see the CPU reset and the system reboot. While this is important for legacy applications (see this blog post for an explanation of why), it's a pain for kernel development, so with the 6.828 patched QEMU you'll instead see a register dump and a "Triple fault." message. 

## E03

阅读 https://pdos.csail.mit.edu/6.828/2018/readings/i386/c09.htm 的笔记在[这里](./interrupts-and-exceptions.md)

## E04

> **Basics of Protected Control Transfer**
>
> Exceptions and interrupts are both "protected control transfers," which cause the processor to switch from user to kernel mode (CPL=0) without giving the user-mode code any opportunity to interfere with the functioning of the kernel or other environments. In Intel's terminology, an interrupt is a protected control transfer that is caused by an asynchronous event usually external to the processor, such as notification of external device I/O activity. An exception, in contrast, is a protected control transfer caused synchronously by the currently running code, for example due to a divide by zero or an invalid memory access.
>
> In order to ensure that these protected control transfers are actually protected, the processor's interrupt/exception mechanism is designed so that the code currently running when the interrupt or exception occurs does not get to choose arbitrarily where the kernel is entered or how. Instead, the processor ensures that the kernel can be entered only under carefully controlled conditions. On the x86, two mechanisms work together to provide this protection:
>  
> 1. **The Interrupt Descriptor Table.** The processor ensures that interrupts and exceptions can only cause the kernel to be entered at a few specific, well-defined entry-points determined by the kernel itself, and not by the code running when the interrupt or exception is taken.
>
>    The x86 allows up to 256 different interrupt or exception entry points into the kernel, each with a different interrupt vector. A vector is a number between 0 and 255. An interrupt's vector is determined by the source of the interrupt: different devices, error conditions, and application requests to the kernel generate interrupts with different vectors. The CPU uses the vector as an index into the processor's interrupt descriptor table (IDT), which the kernel sets up in kernel-private memory, much like the GDT. From the appropriate entry in this table the processor loads:
>
>     - the value to load into the instruction pointer (EIP) register, pointing to the kernel code designated to handle that type of exception.
>  
>     - the value to load into the code segment (CS) register, which includes in bits 0-1 the privilege level at which the exception handler is to run. (In JOS, all exceptions are handled in kernel mode, privilege level 0.)
>
> 2. **The Task State Segment**. The processor needs a place to save the old processor state before the interrupt or exception occurred, such as the original values of EIP and CS before the processor invoked the exception handler, so that the exception handler can later restore that old state and resume the interrupted code from where it left off. But this save area for the old processor state must in turn be protected from unprivileged user-mode code; otherwise buggy or malicious user code could compromise the kernel.
>    
>    For this reason, when an x86 processor takes an interrupt or trap that causes a privilege level change from user to kernel mode, it also switches to a stack in the kernel's memory. A structure called the task state segment (TSS) specifies the segment selector and address where this stack lives. The processor pushes (on this new stack) SS, ESP, EFLAGS, CS, EIP, and an optional error code. Then it loads the CS and EIP from the interrupt descriptor, and sets the ESP and SS to refer to the new stack.
>  
>    Although the TSS is large and can potentially serve a variety of purposes, JOS only uses it to define the kernel stack that the processor should switch to when it transfers from user to kernel mode. Since "kernel mode" in JOS is privilege level 0 on the x86, the processor uses the ESP0 and SS0 fields of the TSS to define the kernel stack when entering kernel mode. JOS doesn't use any other TSS fields.

概念基本都理解, 但是写代码时才发现无从下手. 一是因为不熟悉汇编, 二是没有阅读E03的文档, 导致不清楚具体实现细节, 代码也没有提供足够的示例和注释.

参考了[https://github.com/yinfredyue/MIT6.828/blob/master/labNotes/lab3/lab3A.md](https://github.com/yinfredyue/MIT6.828/blob/master/labNotes/lab3/lab3A.md)才有了一点头绪.

关于中断的笔记: [interrupts-and-exceptions](interrupts-and-exceptions.md)

## E05

页故障(page fault)的中断号是 14, 出现页故障时 CPU 会将出错的虚拟地址保存到 `%cr2` 寄存器, 错误码包含更多出错信息, 具体查看[参考资料](./reference.md).

## E06

challenge 部分要求实现类似 GDB 的 single step 和 continue 功能, 关键是 flag 寄存器的 `TF` bit, `TF` 用于实现 single step. 具体查看[参考资料](./reference.md)的[INTEL 80386 PROGRAMMER'S REFERENCE MANUAL 1986]()的第12章.

## E07

按照要求实现.

challenge

## E08




