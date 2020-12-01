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

## E12

仔细阅读 lab 文档和注释, 注意页表权限.

遇到的问题:

- fork()

  一开始我是在子进程中初始化用户异常栈, 然后 user/forktree.c 的运行结果一直报错(page fault). 原来的代码和报错如下:

  ```
  envid_t
  fork(void)
  {
    // LAB 4: Your code here.
    envid_t envid;
    int err;

    set_pgfault_handler(pgfault);

    envid = sys_exofork();
    if(envid < 0){
      panic("fork: %e", envid);
    }
    if(envid == 0){
      // child
      thisenv = &envs[ENVX(sys_getenvid())];
      // 设置用户异常栈和页故障处理函数.
      set_pgfault_handler(pgfault); 
      return 0;
    }
    ...
  }
  ```


  ```
  ...
  [00000000] new env 00001000
  1000: I am ''
  [00001000] new env 00001001
  [00001000] new env 00001002
  [00001000] exiting gracefully
  [00001000] free env 00001000
  [00001001] user fault va eebfdf48 ip 00800ee4
  TRAP frame at 0xf029c07c from CPU 0
    edi  0x00000000
    esi  0x00000000
    ebp  0xeebfdf70
    oesp 0xefffffdc
    ebx  0x00000000
    edx  0x00000000
    ecx  0x008010a9
    eax  0x00000000
    es   0x----0023
    ds   0x----0023
    trap 0x0000000e Page Fault
    cr2  0xeebfdf48
    err  0x00000007 [user, write, protection]
    eip  0x00800ee4
    cs   0x----001b
    flag 0x00000092
    esp  0xeebfdf4c
    ss   0x----0023
  [00001001] free env 00001001
  ...
  ```

  出错原因是因为当子进程开始运行后, 立刻就会往栈上写数据, 导致 COW 页故障, 然而此时子进程还来不及设置页故障处理函数, 因此导致页故障无法被正常处理. 报错位置如下:

  ```
  // LAB 4: Your code here.
	if(curenv->env_pgfault_upcall == NULL){
		// Destroy the environment that caused the fault.
		cprintf("[%08x] user fault va %08x ip %08x\n",
			curenv->env_id, fault_va, tf->tf_eip);
		print_trapframe(tf);
		env_destroy(curenv);
		return;
	}
  ```

  解决办法就是在父进程先设置好子进程的用户异常栈和页故障处理函数.

- uvpd 和 uvpt

  一开始只单独使用 uvpt 查询虚拟地址的页表项. 这其实是不安全的, 应该先使用 uvpd 判断二级页表是否存在.

  错误用法:

  ```
  if(uvpt[PGNUM(va)] & some_permissions){
    ...
  }
  ```

  正确用法:

  ```
  if(uvpd[PDX(va)] & PTE_P && uvpt[PGNUM(va)] & some_permissions){
    ...
  }
  ```


## E13

按照要求实现就好.


遇到的问题:

- 运行`make run-spin-nox` 失败.

  原因是 trap_init() 设置中断描述符表的 一个bug.

  在 trap_init() 中通过 `SETGATE` 宏设置中断描述符表, 我原来将 T_DEBUG, T_BRKPT, T_OFLOW, T_SYSCALL 的 istrap 参数设置为 1, 导致出错:

  ```
  kernel panic on CPU 0 at kern/trap.c:306: assertion failed: !(read_eflags() & FL_IF)
  ```

  其实所有的中断的 istrap 参数都为 0. 这样 CPU 在切换到内核模式时才会清除 eflags 的 FL_IF 标志位.

## E14

lab 文档说完成 E14 后, `make grade` 的分数应该是 65/80, 然后此时我的分数只有 45/80, 通过检查发现下面的问题:

- `faultregs` 测试失败.

  原因是 pfentry.S 中的 bug.

  ```
  ...
  // Restore eflags from the stack.  After you do this, you can
	// no longer use arithmetic operations or anything else that
	// modifies eflags.
	// LAB 4: Your code here.
	addl $4, %esp
	popfl

	// Switch back to the adjusted trap-time stack.
	// LAB 4: Your code here.
	popl %esp
  subl $4, %esp // 错误操作
  ...
  ```

  上面注释中明确说了在恢复 eflags 后, 不能再进行算术运算, 否则会修改 eflags. 因此 `subl $4, %esp` 是错误的, 应该在一开始就直接修改 utf 的 %esp.

- `faultnostack` 测试失败.

  原因是 trap.c 中的 page_fault_handler() 函数的一个 bug.

  修改一下 user_mem_assert 就可以了.

  ```
  $ git diff kern/trap.c

  +       uint32_t sz = sizeof(struct UTrapframe);
 
  -       user_mem_assert(curenv, (void *)uxstackbottom, PGSIZE, PTE_U | PTE_W | PTE_P);
  -       
          if(tf->tf_esp >= uxstackbottom && tf->tf_esp < UXSTACKTOP){
                  // recursive page fault
  -               uxesp = tf->tf_esp - sizeof(uintptr_t);
  +               uxesp = tf->tf_esp;
  +               sz += sizeof(uintptr_t);
          }
          // set up exception stack
  -       uxesp -= sizeof(struct UTrapframe);
  -       if(uxesp < uxstackbottom) {
  -               // Destroy the environment that caused the fault.
  -               cprintf("[%08x] user fault va %08x ip %08x\n"
  -                               "UXSTACK overflow\n",
  -                               curenv->env_id, fault_va, tf->tf_eip);
  -               print_trapframe(tf);
  -               env_destroy(curenv);
  -               return;
  -       }
  +       uxesp -= sz;
  +       user_mem_assert(curenv, (void *)uxesp, sz , PTE_U | PTE_W | PTE_P);
  ```

## E15

按照注释和 lab 文档仔细实现各种要求.

在调试 bug 时反复浏览了几遍代码, 修复了之前代码的几个 bug.


- sys_page_alloc() 对 perm 参数的检查存在 bug.

  ```diff
  --- a/kern/syscall.c
  +++ b/kern/syscall.c
  @@ -176,9 +176,12 @@ sys_page_alloc(envid_t envid, void *va, int perm)
          if((uintptr_t)va >= UTOP || PGOFF(va)){
                  return -E_INVAL;
          }
  -       if(perm & (~PTE_SYSCALL)){
  +       
  +       int req_perm = PTE_U | PTE_P;
  +       if((perm & req_perm) != req_perm || perm & (~PTE_SYSCALL)){
                  return -E_INVAL;
          }
  +
          struct Env *e;
          int err;
          struct PageInfo *pp;
  ```

- page_insert() 检查页表项权限的 bug. 非常低级的一个 bug, 把 "&" 写成了 "|".

  ``` diff
  --- a/kern/pmap.c
  +++ b/kern/pmap.c
  @@ -505,11 +505,11 @@ page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
          if(pte == NULL){
                  return -E_NO_MEM;
          }
  -       if(*pte | PTE_P && PTE_ADDR(*pte) == PTE_ADDR(page2pa(pp))){
  -               *pte = PTE_ADDR(*pte) | perm | PTE_P;
  -               return 0;
  -       }
  -       if(*pte | PTE_P){
  +       if(*pte & PTE_P){
  +               if(PTE_ADDR(*pte) == PTE_ADDR(page2pa(pp))){
  +                       *pte = PTE_ADDR(*pte) | perm | PTE_P;
  +                       return 0;
  +               }
                  page_remove(pgdir, va);
          }
          *pte = PTE_ADDR(page2pa(pp)) | perm | PTE_P;
  ```

- env_run() 的一个 bug.

  ``` diff
  --- a/kern/env.c
  +++ b/kern/env.c
  @@ -536,7 +536,7 @@ env_run(struct Env *e)
          //      e->env_tf to sensible values.
  
          // LAB 3: Your code here.
  -       if(curenv != NULL){
  +       if(curenv && curenv->env_status == ENV_RUNNING){
                  curenv->env_status = ENV_RUNNABLE;
          }
          curenv = e;
  ```

  导致 primes.c 的测试失败. 错误过程复现:

  - 进程 A 调用 ipc_recv 接受数据, 进入阻塞状态, env_status 等于 ENV_NOT_RUNNABLE, 并调用 sched_yield() 进行调度.

  - sched_yield() 挑选一个进程, 接着调用 env_run() 运行它. 
  
  - env_run() 将 curenv 的状态设置为 ENV_RUNNABLE, 此时的 curenv 是进程 A, 导致进程 A 有可能在下次时钟中断时被内核调度运行. 
  
    trick 的地方就在于, 对于 primes 这样生成大量进程的程序来说, 容易导致进程 A 在未实际接受到别的进程发送给它的数据之前就被内核调度, 导致数据不一致的 bug.

    具体的错误就是系统调用 sys_ipc_recv() 的返回值(保存在 %eax)等于 12, 12 就是 sys_ipc_recv() 的系统调用的编号.

    ```
    $ make run-primes-nox
    ...
    [00000000] new env 00001000
    [00001000] new env 00001001
    CPU 0: 2 [00001001] new env 00001002
    CPU 0: 3 [00001002] new env 00001003
    [00001002] user panic in <unknown> at lib/syscall.c:35: syscall 12 returned 12 (> 0)
    ```

user/primes.c 的基本原理是素数筛, 利用进程实现并发.

