# Lab 6

## E01

...

## E02

...

## E03

82540EM 的 vendor_id = 0x8086, device_id = 0x100e.

## E04

13.4.2 定义了 device status register 各个字段的含义.

相关常量可以在 qemu 的 e1000_hw.h 中找到.


## E05

从这里开始变难了.

14.5 描述了初始化流程

3.3.3 描述了 Legacy Transmit Descriptor Format.

3.4 描述了 Transmit Descriptor Ring Structure.

13.2 描述了 E1000 寄存器的位置.

13.4.33 描述了 Transmit Control Register


transmit descriptor 数组是一个循环数组, 每个 transmit descriptor 大小为 16 字节. 文档要求数组长度(TDLEN, 以字节为单位)对齐 128 字节, 因此数组长度必须为 8 的倍数.

transmit descriptor 数组的地址必须 16 字节对齐, 文档说网卡驱动会忽略低位的 4 个 bit. 

网卡驱动使用 DMA 操作内存, 因此在分配数据包的缓冲区时, 保证缓冲区的物理地址是连续的.

网卡驱动根据头尾指针判断输出缓冲队列(FIFO)是否为空, 如果头尾指针相等, 说明输出缓冲队列为空. 另外, 根据文档描述, 如果 td_desc.addr = 0, 或者 td_desc.len = 0, 那么将不会传输该数据包.

td_desc.cmd 中有两个比较重要的标志位, RS 和 EOP, RS 告诉网卡驱动在发送了该数据包, 设置 td_desc.status 字段的 DD 标志位. EOP 告诉网卡驱动该数据包是最后一个数据包, 从而网卡驱动可以开始发送数据. 如果不设置 EOP 那么网卡驱动不会发送数据包, 这是我写代码时的 bug.


## E06

注意设置 td_desc.cmd 的 EOP 标志位.

检查队列是否已满.


## E07

...

## E08

output 是一个事件循环:

- ipc_recv, 等待上层网络栈的 NSREQ_OUTPUT 事件.

- 调用 sys_net_tx_packet 将数据包发送出去.

  如果 sys_net_tx_packet 返回 -E_NETWORK_NO_BUF, 说明输出队列已满, 那么调用 sys_yield 进入睡眠, 然后重试. 最大重试次数等于 MAXTRY.

  如果一直阻塞在 sys_net_tx_packet, 那么上层网络栈也将一直阻塞在 ipc_send, 等待 output 接受 ipc request.


## E09

...

## E10

14.4 描述了初始化流程

3.2 描述了 Receive Descriptor.

13.4 和 13.5 描述了相关寄存器.


receive descriptor 数组和 transmit descriptor 类似.

头尾指针相等表明接收队列已满.

初始化时头指针等于1, 尾指针等于0.


## E11

检查 rx_desc.status 字段判断缓冲是否有数据包.

## E12

```C
// Hint: When you IPC a page to the network server, it will be
// reading from it for a while, so don't immediately receive
// another packet in to the same physical page.
```
上面的注释说了, 当通过 IPC 发送数据包给上层网络栈后, 不要马上读取下一个数据包, 否则会覆盖上一个数据包的数据.

解决方法是每次读取数据包时都重新申请一个物理页, 然后将读取的新数据包保存到新申请的物理页, 然后使用 ipc_send 将新物理页发送给上层网络栈.
此时如果上层网络栈还在处理旧数据包, 那么 ipc_send 会阻塞, 知道上层网络栈处理完旧数据包. 

这个方法并不会无限制地一直消耗内存导致内存泄漏. 在正常情况下, input 申请一个物理页, 物理页的引用计数加 1, 使用 ipc_send 将该物理页发送给上层网络栈, 引用计数再加 1.
当 input 再申请一个新物理页, 旧物理页的引用计数减 1, 新物理页的引用计数加 1. 当上层网络栈接受 ipc 的新物理页时,  旧物理页的引用计数减 1, 新物理页的引用计数加 1. 从而释放旧物理页.

因此最多只会消耗两个物理页.

## E13

按照要求实现.

遇到一个意料之外的问题, 用户栈溢出导致 page fault.

```c
Waiting for http connections...
[00001002] user fault va eebfccb0 ip 008000d0
TRAP frame at 0xf02f90f8 from CPU 0
  edi  0xeebfdf94
  esi  0x00000001
  ebp  0xeebfdf80
  oesp 0xefffffdc
  ebx  0x00000001
  edx  0x00000073
  ecx  0xd0001000
  eax  0x00000001
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xeebfccb0
  err  0x00000006 [user, write, not-present]
  eip  0x008000d0
  cs   0x----001b
  flag 0x00000286
  esp  0xeebfccb4
  ss   0x----0023
```

出错的 va 等于 0xeebfccb0, 用户进程在初始化默认只分配一个page作为栈, 栈的底部地址等于 0xeebfd000. 

出错的代码位置在 handle_client 函数. 汇编代码如下:

```asm
static void
handle_client(int sock)
{
  8000c2:	55                   	push   %ebp
  8000c3:	89 e5                	mov    %esp,%ebp
  8000c5:	57                   	push   %edi
  8000c6:	56                   	push   %esi
  8000c7:	53                   	push   %ebx
  8000c8:	81 ec c0 12 00 00    	sub    $0x12c0,%esp
  8000ce:	89 c6                	mov    %eax,%esi
	struct http_request *req = &con_d;
	
	while (1)
	{
		// Receive message
		if ((received = read(sock, buffer, BUFFSIZE)) < 0)
  8000d0:	68 00 02 00 00       	push   $0x200
  8000d5:	8d 85 dc fd ff ff    	lea    -0x224(%ebp),%eax
  8000db:	50                   	push   %eax
  8000dc:	56                   	push   %esi
  8000dd:	e8 50 17 00 00       	call   801832 <read>
```

从 trapframe 可以看出此时 %ebp = 0xeebfdf80, 距离栈底 0xeebfd000 还有 0xf80 这么多空间, 足够 handle_client 函数使用了. 
不过汇编代码中 `sub    $0x12c0,%esp` 指令将 %esp 减少了 0x12c0, 导致栈溢出.

我猜这可能是 gcc 的某种优化.

后来, 我写了一个专门处理栈溢出的 page fault 处理函数, 在 libmain 中 `set_pgfault_handler(default_pgfault_handler);`.



