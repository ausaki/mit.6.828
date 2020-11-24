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


## E05





