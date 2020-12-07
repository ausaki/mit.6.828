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

初始化流程:

Allocate a region of memory for the transmit descriptor list. Software should insure this memory is aligned on a paragraph (16-byte) boundary. 

Program the Transmit Descriptor Base Address (TDBAL/TDBAH) register(s) with the address of the region. TDBAL is used for 32-bit addresses and both TDBAL and TDBAH are used for 64-bit addresses.

Set the Transmit Descriptor Length (TDLEN) register to the size (in bytes) of the descriptor ring. This register must be 128-byte aligned.

The Transmit Descriptor Head and Tail (TDH/TDT) registers are initialized (by hardware) to 0b after a power-on or a software initiated Ethernet controller reset. Software should write 0b to both these registers to ensure this.

Initialize the Transmit Control Register (TCTL) for desired operation to include the following: 

- Set the Enable(TCTL.EN) bit to 1b for normal operation.

- Set the Pad Short Packets(TCTL.PSP) bit to 1b.

- Configure the Collision Threshold(TCTL.CT) to the desired value. Ethernet standard is 10h. This setting only has meaning in half duplex mode.

- Configure the Collision Distance (TCTL.COLD) to its expected value. For full duplex operation, this value should be set to 40h. For gigabit half duplex, this value should be set to 200h. For 10/100 half duplex, this value should be set to 40h.


3.3.3 描述了 Legacy Transmit Descriptor Format.

3.4 描述了 Transmit Descriptor Ring Structure.

13.2 描述了 E1000 寄存器的位置.

13.4.33 描述了 Transmit Control Register

