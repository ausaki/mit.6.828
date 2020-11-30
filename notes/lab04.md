# Lab 04

## E01

...

## E02

...

### Q1 

因为 bootloader 的load adress 和 linker address 是一样的, 由下图可知都是0x7c00.

```
$ readelf -l obj/boot/boot.out 

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x000074 0x00007c00 0x00007c00 0x00244 0x00244 RWE 0x4
  GNU_STACK      0x000000 0x00000000 0x00000000 0x00000 0x00000 RWE 0x10

 Section to Segment mapping:
  Segment Sections...
   00     .text .eh_frame 
   01     
```

mpentry.S 是和内核一起加载的, 通过 init.c/boot_aps 函数将 mpentry.S 的代码复制到 MPENTRY_PADDR. mpentry.S 的 load address 和 linker address 是不一样的, 所以需要通过  MPBOOTPHYS 宏将地址转换为实际的绝对地址.

## E03

...

## E04

...

## E05

...

## E06

...

### Q03

env->env_pgdir 是从 kern_pgdir 复制过来的, KERNBASE 以上的虚拟内存是一样的, 所以切换页表前后都可以访问参数 e.

### Q04

当用户环境调用系统调用 `sys_yield` 时, 进行上下文切换, 进入内核模式, 内核在 trap() 函数中将 trapframe 保存到 curenv->env_trapframe. 如下:

```
  // Copy trap frame (which is currently on the stack)
  // into 'curenv->env_tf', so that running the environment
  // will restart at the trap point.
  curenv->env_tf = *tf;
  // The trapframe on the stack should be ignored from here on.
  tf = &curenv->env_tf;
```

## E07

...


## E08

...

## E09

仔细按照 `UTrapframe` 的结构来设置栈.

以下代码实现跳转到用户页故障处理代码.

```
tf->tf_esp = uxesp;
tf->tf_eip = (uintptr_t)curenv->env_pgfault_upcall;
```

## E10

这个练习需要写汇编代码, 而且需要特别理解汇编的一些技巧.

pfentry.S 的注释已经提到一些需要注意的点, 下面是一些总结:

- 不能使用 `jmp` 指令跳转到发生页故障的用户代码, 应为 `jmp` 需要使用一个通用寄存器来保存跳转地址, 而这可能会破环通用寄存器的数据一致性.

- `popal` 指令用于从当前 %esp 取出通用寄存器并覆盖原有的值.

- `popfl` 指令用于从当前 %esp 取出 eflags 并覆盖原有的值.

- `pfentry.S` 的注释中实现跳转的解决方法大致如下:

  - 将 trap time 的 %eip(utf->utf_eip) 保存到 trap time 栈(即发生页故障时的栈).

  - 恢复寄存器.
  
  - 此时 %esp 指向 trap time 栈, 此处保存的值正好是 %eip. 然后使用 `ret` 指令恢复 %eip.

  - 如果发生递归的页故障, 即用户处理页故障的代码也发生了页故障, 从而导致递归页故障. 那么需要在用户异常栈预留 4 个字节的空间用于保存 %eip.

## E11

...



