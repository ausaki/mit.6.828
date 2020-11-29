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