# Lab5

合并代码后测试代码是否运行正常发现两个问题.

- `make run-primes-nox` 编译出错.

  ``` console
  lib/spawn.c:110:35: error: taking address of packed member of ‘struct Trapframe’ may result in an unaligned pointer value [-Werror=address-of-packed-member]
  110 |  if ((r = init_stack(child, argv, &child_tf.tf_esp)) < 0)
  ```

  网上搜索后得知这个错误是在高版本的 GCC 出现的. 解决方法是删除 `-Werror` 选项, 这个选项在项目根目录的 GNUmakefile 文件中. 我的做法是修改 init_stack 函数的代码.

  ``` diff
  diff --git a/lib/spawn.c b/lib/spawn.c
  index 9d0eb07..fda1903 100644
  --- a/lib/spawn.c
  +++ b/lib/spawn.c
  @@ -6,7 +6,7 @@
  #define UTEMP3                 (UTEMP2 + PGSIZE)
  
  // Helper functions for spawn.
  -static int init_stack(envid_t child, const char **argv, uintptr_t *init_esp);
  +static int init_stack(envid_t child, const char **argv, struct Trapframe *tf);
  static int map_segment(envid_t child, uintptr_t va, size_t memsz,
                        int fd, size_t filesz, off_t fileoffset, int perm);
  static int copy_shared_pages(envid_t child);
  @@ -107,7 +107,7 @@ spawn(const char *prog, const char **argv)
          child_tf = envs[ENVX(child)].env_tf;
          child_tf.tf_eip = elf->e_entry;
  
  -       if ((r = init_stack(child, argv, &child_tf.tf_esp)) < 0)
  +       if ((r = init_stack(child, argv, &child_tf)) < 0)
                  return r;
  
          // Set up program segments as defined in ELF header.
  @@ -184,7 +184,7 @@ spawnl(const char *prog, const char *arg0, ...)
  // to the initial stack pointer with which the child should start.
  // Returns < 0 on failure.
  static int
  -init_stack(envid_t child, const char **argv, uintptr_t *init_esp)
  +init_stack(envid_t child, const char **argv, struct Trapframe *tf)
  {
          size_t string_size;
          int argc, i, r;
  @@ -244,7 +244,7 @@ init_stack(envid_t child, const char **argv, uintptr_t *init_esp)
          argv_store[-1] = UTEMP2USTACK(argv_store);
          argv_store[-2] = argc;
  
  -       *init_esp = UTEMP2USTACK(&argv_store[-2]);
  +       tf->tf_esp = UTEMP2USTACK(&argv_store[-2]);
  
          // After completing the stack, map it into the child's address space
          // and unmap it from ours!

  ```

- 这个错误是在修复了上一个错误后接着出现的, 链接器发现重复定义的变量.

  ``` console
  + ld obj/fs/fs
  ld: obj/fs/bc.o: in function `bc_pgfault':
  fs/bc.c:31: multiple definition of `super'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/bc.o: in function `bc_pgfault':
  fs/bc.c:31: multiple definition of `bitmap'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/fs.o: in function `walk_path':
  fs/fs.c:233: multiple definition of `super'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/fs.o: in function `walk_path':
  fs/fs.c:233: multiple definition of `bitmap'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/serv.o: in function `serve_read':
  fs/serv.c:218: multiple definition of `bitmap'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/serv.o: in function `serve_read':
  fs/serv.c:218: multiple definition of `super'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/test.o: in function `fs_test':
  fs/test.c:10: multiple definition of `bitmap'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ld: obj/fs/test.o: in function `fs_test':
  fs/test.c:10: multiple definition of `super'; obj/fs/ide.o:fs/ide.c:19: first defined here
  ```

  原因是在 fs/fs.h 头文件中定义了 super 和 bitmap 变量, 而且多个源文件引用了这个头文件, 从而加载器发现多个重复定义的变量. 早期版本的加载器默认选择其中一个定义.

  解决方法有两种, 一是添加 -fcommon 编译选项, 而是修改代码. 我选择修改代码.

  ```diff
  diff --git a/fs/fs.c b/fs/fs.c
  index 45ecaf8..fb59595 100644
  --- a/fs/fs.c
  +++ b/fs/fs.c
  @@ -3,6 +3,9 @@
  
  #include "fs.h"
  
  +struct Super *super;           // superblock
  +uint32_t *bitmap;              // bitmap blocks mapped in memory
  +
  // --------------------------------------------------------------
  // Super block
  // --------------------------------------------------------------
  diff --git a/fs/fs.h b/fs/fs.h
  index 0350d78..9d4a21a 100644
  --- a/fs/fs.h
  +++ b/fs/fs.h
  @@ -11,8 +11,8 @@
  /* Maximum disk size we can handle (3GB) */
  #define DISKSIZE       0xC0000000
  
  -struct Super *super;           // superblock
  -uint32_t *bitmap;              // bitmap blocks mapped in memory
  +extern struct Super *super;            // superblock
  +extern uint32_t *bitmap;               // bitmap blocks mapped in memory
  
  /* ide.c */
  bool   ide_probe_disk1(void);
  ```


这次 lab 主要是关于文件系统和shell, 大部分的代码都已经提供了, 要求实现的代码比较少, 按照注释实现就行. 

通过阅读文件系统的代码知道了底层是如何读写磁盘的. 


