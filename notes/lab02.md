# Lab02

## E01

实现 `boot_alloc` 时注意地址需要按照 PGSIZE 对齐和内存溢出.

实现 `page_init` 代码之前需要理解 kernel 在启动阶段的物理内存布局，根据 `page_init` 函数中的提示：

```
//  1) Mark physical page 0 as in use.
	//     This way we preserve the real-mode IDT and BIOS structures
	//     in case we ever need them.  (Currently we don't, but...)
	//  2) The rest of base memory, [PGSIZE, npages_basemem * PGSIZE)
	//     is free.
	//  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM), which must
	//     never be allocated.
	//  4) Then extended memory [EXTPHYSMEM, ...).
	//     Some of it is in use, some is free. Where is the kernel
	//     in physical memory?  Which pages are already in use for
	//     page tables and other data structures?
```

可以大致了解除了 kernel 之外内存布局，kernel 占据的内存从 1MB 开始到 end(查看 kern/entry.S)。

另外, 从 `boot_alloc` 函数可以知道, nextfree 是从 end 开始分配内存的, 所以还必须考虑 `boot_alloc` 分配掉的内存, nextfree 的值可以通过执行 `boot_alloc(0)` 获得.

代码: [../kern/pmap.c](../kern/pmap.c)


## E02

## E03


## E04

写这个练习的几个函数时, 非常容易被虚拟地址(va)和物理地址(pa)搞晕, 需要时刻保持清醒.

由于 lab2 的文档里面没有提供这些函数的具体实现细节, 需要自己阅读代码和注释, 然后推理除各个函数要实现的功能. 另外, 通过阅读对应的测试函数(`check_xxx`)也非常有帮助.

发现了`pgdir_walk`函数的注释中的一个小错误: 

```
/ The relevant page table page might not exist yet.
// If this is true, and create == false, then pgdir_walk returns NULL.
// Otherwise, pgdir_walk allocates a new page table page with page_alloc.
//    - If the allocation fails, pgdir_walk returns NULL.
//    - Otherwise, the new page's reference count is incremented,
//	the page is cleared,
//	and pgdir_walk returns a pointer into the new page table page.
//	上面这句改为: and pgdir_walk returns a pointer to the page talbe entry in the new page table.  
//
```

其实, 无论page table是否已经存在, 都应该返回指向对应 PTE 的指针. 一开始我按照原来的注释返回指向 new page table 的指针, 导致`check_page`的测试出错. 出错的代码如下:

```
  // check pointer arithmetic in pgdir_walk
  page_free(pp0);
  va = (void*)(PGSIZE * NPDENTRIES + PGSIZE);
  ptep = pgdir_walk(kern_pgdir, va, 1);
  ptep1 = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(va)]));
  assert(ptep == ptep1 + PTX(va));  // assert 出错
  kern_pgdir[PDX(va)] = 0;
  pp0->pp_ref = 0;
```

代码: [../kern/pmap.c](../kern/pmap.c)

### 页表

地址空间为 32bit, 高 10bit 表示 page directory(即一级页表) 索引, 次 10bit 表示 page table(即二级页面) 索引, 最低 12bit 表示地址偏移.

一级页表和二级页表都是4KB, 可以存储1024个条目, 每个条目4字节. 一级页表的条目保存着指向二级页表的物理地址. 条目的最低 10bit 用于存储访问权限.

%cr3 寄存器指向 page directory.

## E05

使用 `boot_map_region`.


## Q03

> We have placed the kernel and user environment in the same address space. Why will user programs not be able to read or write the kernel's memory? What specific mechanisms protect the kernel memory?

页表项的低 12bit 保存了页的访问权限, MMU 在翻译地址时会检查权限. 另外, CPU 还会使用一个模式标志位(%cr0 的 CPL)来区分当前是用户模式还是内核模式. 

具体可以参考: 

- [页表, 地址翻译和控制寄存器](https://pdos.csail.mit.edu/6.828/2018/lec/x86_translation_and_registers.pdf)

- [页表, 地址翻译](https://pdos.csail.mit.edu/6.828/2018/lec/l-vm.pdf)


## Q04

>  What is the maximum amount of physical memory that this operating system can support? Why?

根据 `pmap.c` 的代码可以知道, `pages` 被映射到 `UPAGES` 区域, `UPAGES` 区域大小等于 `PTSIZE`, 即 4MB, 每个 `struct PageInfo` 结构等于 8 bytes, 因此总的可以保存 `2 ^ 22 / 2 ^ 3 = 2 ^ 19 = 512K` 个 `struct PageInfo`. 所以支持的最大内存等于 `512K * 4KB = 2GB`.

## Q05

>  How much space overhead is there for managing memory, if we actually had the maximum amount of physical memory? How is this overhead broken down?

- 一级页表 = 4KB

- 一个二级页表 = 4KB, 最多 1K 个二级页表, 总的 = 4MB.

- pages 链表 (`UPAGES`) 最多 4MB.

## Q06

> Revisit the page table setup in kern/entry.S and kern/entrypgdir.c. Immediately after we turn on paging, EIP is still a low number (a little over 1MB). At what point do we transition to running at an EIP above KERNBASE? What makes it possible for us to continue executing at a low EIP between when we enable paging and when we begin running at an EIP above KERNBASE? Why is this transition necessary? 

开启分页机制:

```
# Load the physical address of entry_pgdir into cr3.  entry_pgdir
# is defined in entrypgdir.c.
movl	$(RELOC(entry_pgdir)), %eax
movl	%eax, %cr3
# Turn on paging.
movl	%cr0, %eax
orl	$(CR0_PE|CR0_PG|CR0_WP), %eax
movl	%eax, %cr0

# Now paging is enabled, but we're still running at a low EIP
# (why is this okay?).  Jump up above KERNBASE before entering
# C code.
mov	$relocated, %eax
jmp	*%eax
```

`entrypgdir` 将高地址的 `[KERNBASE, KERNBASE + 4MB)` 和低地址的 `[0, 4MB)` 都映射到物理地址 `[0, 4MB)`. 因此在这个极段的过渡期内可以同时使用高地址的 eip 和低地址的 eip



## 其它

- UVPT的作用?

  见 https://pdos.csail.mit.edu/6.828/2018/lec/l-josmem.html.



