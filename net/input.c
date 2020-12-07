#include "ns.h"

extern union Nsipc nsipcbuf;
static struct jif_pkt *pkt = (struct jif_pkt*)REQVA;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r, perm;
	perm = PTE_U | PTE_W | PTE_P;

	while(1){
		if((r = sys_page_alloc(0, pkt, perm)) < 0){
			sys_yield();
			continue;
		}
		if((r = sys_net_rx_packet(pkt->jp_data, PGSIZE - sizeof(int))) < 0){
			sys_yield();
			continue;
		}
		pkt->jp_len = r;
		ipc_send(ns_envid, NSREQ_INPUT, pkt, perm);
		sys_yield();
	}
}
